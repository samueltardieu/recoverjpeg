#![deny(clippy::pedantic)]

use clap::Parser;
use indicatif::{ProgressBar, ProgressState, ProgressStyle};
use mov::is_valid_atom_type;
use std::fs::File;
use std::io::{ErrorKind, Read, Seek, SeekFrom, Write};

mod mov;
mod utils;

type AtomType = [u8; 4];

#[derive(Parser)]
#[clap(author, version, about, long_about = None)]
struct Args {
    /// Block size in bytes
    #[clap(
        short,
        long,
        value_parser = utils::bytes_with_suffix,
        default_value_t = 512,
    )]
    block_size: u64,
    /// Base name of the mov files to create
    #[clap(short = 'n', long, default_value_t = String::from("video_"))]
    base: String,
    /// Do not write recovered files
    #[clap(long)]
    dry_run: bool,
    /// Initial movie index
    #[clap(short, long, default_value_t = 1)]
    index: usize,
    /// Maximum chunk size in MiB [default: 100]
    #[clap(
        short,
        long,
        default_value_t = 100 * 1024 * 1024,
        hide_default_value = true,
        value_parser = utils::mib_with_suffix,
    )]
    max_chunk_size: u64,
    /// Restore mov files into this directory
    #[clap(short = 'o', long)]
    directory: Option<String>,
    /// Show progress indicator
    #[clap(short, long)]
    progress: bool,
    /// File or device to restore from
    file: String,
}

fn read_size_and_atom_type(fd: &mut File) -> std::io::Result<(u32, AtomType)> {
    let mut buffer = [0u8; 8];
    fd.read_exact(&mut buffer)?;
    fd.seek(SeekFrom::Current(-8))?;
    Ok((
        u32::from_be_bytes(buffer[..4].try_into().unwrap()),
        buffer[4..].try_into().unwrap(),
    ))
}

fn read_chunk(
    fd: &mut File,
    dst: &mut Vec<u8>,
    max_chunk_size: u64,
) -> std::io::Result<Option<AtomType>> {
    let (size, atom_type) = read_size_and_atom_type(fd)?;
    if !(is_valid_atom_type(atom_type) && (8..=max_chunk_size).contains(&u64::from(size))) {
        return Ok(None);
    }
    dst.resize(size as usize, 0);
    fd.read_exact(&mut dst[..size as usize])?;
    Ok(Some(atom_type))
}

fn decode(args: &Args, fd: &mut File, recovered: &mut usize) -> std::io::Result<usize> {
    let mut buffer = Vec::new();
    let max_chunk_size = args.max_chunk_size;
    let dry_run_mode = if args.dry_run { " (dry-run mode)" } else { "" };
    let bar = args.progress.then(|| {
        use std::fmt::Write;
        let file_size = fd.metadata().unwrap().len();
        let pb = ProgressBar::new(file_size);
        pb.set_style(ProgressStyle::with_template("{spinner:.green} [{elapsed_precise}] [{wide_bar:.cyan/blue}] {bytes}/{total_bytes} ({eta})")
        .unwrap()
        .with_key("eta", |state: &ProgressState, w: &mut dyn Write| write!(w, "{:.1}s", state.eta().as_secs_f64()).unwrap())
        .progress_chars("#>-"));
        pb
    });
    let mut block = 0;
    loop {
        let pos = block * args.block_size;
        if let Some(pb) = &bar {
            pb.set_position(pos);
        }
        fd.seek(SeekFrom::Start(pos))?;
        if let Some(mut atom_type) = read_chunk(fd, &mut buffer, max_chunk_size)? {
            if &atom_type == b"ftyp" && &buffer[8..12] == b"qt  " {
                log::info!("Found MOV file at offset {}", pos);
                let mut out_file = format!("{}{}.mov", args.base, args.index + *recovered);
                if let Some(dir) = args.directory.as_deref() {
                    out_file = format!("{dir}/{out_file}");
                }
                log::debug!("Creating out file {out_file}{dry_run_mode}");
                let mut out_file = (!args.dry_run)
                    .then(|| File::create(&out_file))
                    .transpose()?;
                *recovered += 1;
                let mut recovered_size = 0;
                loop {
                    log::trace!(
                        "Copying {} bytes of type {}{dry_run_mode}",
                        buffer.len(),
                        unsafe { std::str::from_utf8_unchecked(&atom_type) }
                    );
                    out_file
                        .as_mut()
                        .map(|f| f.write_all(&buffer))
                        .transpose()?;
                    recovered_size += buffer.len();
                    atom_type = match read_chunk(fd, &mut buffer, max_chunk_size) {
                        Ok(Some(at)) => at,
                        Ok(None) => break,
                        Err(e) if e.kind() == ErrorKind::UnexpectedEof => break,
                        Err(e) => return Err(e),
                    };
                }
                log::debug!("Size of recovered file: {recovered_size} bytes");
                block += (recovered_size - 1) as u64 / args.block_size;
            }
        }
        block += 1;
    }
}

fn main() -> std::io::Result<()> {
    let args = Args::parse();
    pretty_env_logger::init();
    let mut fd = File::open(&args.file)?;
    let mut recovered = 0;
    let r = decode(&args, &mut fd, &mut recovered).unwrap_err();
    log::info!("Files recovered: {recovered}");
    if r.kind() == ErrorKind::UnexpectedEof {
        Ok(())
    } else {
        Err(r)
    }
}
