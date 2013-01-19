/*
 * This file is part of the recoverjpeg program.
 *
 * Copyright (c) 2004-2012 Samuel Tardieu <sam@rfc1149.net>
 * http://www.rfc1149.net/devel/recoverjpeg
 *
 * recoverjpeg is released under the GNU General Public License
 * version 2 that you can find in the COPYING file bundled with the
 * distribution.
 */

/* Needed on Linux to work on large files or devices */

#define _FILE_OFFSET_BITS 64

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

static int verbose = 0;
static int quiet = 0;
static size_t max_size = 6 * 1024 * 1024;
static size_t ignore_size = 0;

static void
usage(int clean_exit)
{
  fprintf(stderr, "Usage: recoverjpeg [options] file|device\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "   -b blocksize   Block size in bytes "
	  "(default: 512)\n");
  fprintf(stderr, "   -d format      Directory format string in printf syntax\n");
  fprintf(stderr, "   -f format      File format string in printf syntax\n");
  fprintf(stderr, "   -h             This help message\n");
  fprintf(stderr, "   -i index       Initial picture index\n");
  fprintf(stderr, "   -m maxsize     Max jpeg file size in bytes "
	  "(default: 6m)\n");
  fprintf(stderr, "   -q             Be quiet\n");
  fprintf(stderr, "   -r readsize    Size of disk reads in bytes "
	  "(default: 128m)\n");
  fprintf(stderr, "   -s cutoff      Minimal file size in bytes to restore\n");
  fprintf(stderr, "   -v verbose     Replace progress bar by details\n");
  fprintf(stderr, "   -V             Display version and exit\n");
  exit(clean_exit ? 0 : 1);
}

static inline int
progressbar()
{
  return !(quiet || verbose);
}

static void
display_progressbar(off_t offset, unsigned int n)
{
  off_t to_display;
  static unsigned int old_n = -1;
  static int gib_mode = 0;
  static off_t old_to_display = 0.0;

  if (offset < 1024 * 1024 * 1024) {
    to_display = offset / 1024 * 10 / 1024;
  }
  else {
    gib_mode = 1;
    to_display = offset / (1024 * 1024) * 10 / 1024;
  }

  if (n != old_n || to_display != old_to_display) {
    printf("\rRecovered files: %4u        Analyzed: %4.1f %s  ",
	   n, to_display / 10.0, gib_mode ? "GiB" : "MiB");
    fflush(stdout);
    old_n = n;
    old_to_display = to_display;
  }
}

static void
cleanup_progressbar()
{
  printf("\r                                                     \r");
}

static size_t
jpeg_size(const unsigned char *start)
{
  const unsigned char *addr;
  unsigned char code;
  size_t size;

  if (*start != 0xff || *(start + 1) != 0xd8) {
    return 0;
  }

  if (verbose) {
    fprintf(stderr, "Candidate jpeg found\n");
  }

  for (addr = start + 2;;) {
    if (*addr != 0xff) {
      if (verbose) {
	fprintf(stderr,
		"   Incorrect marker %02x, stopping prematurely\n", *addr);
      }
      return 0;
    }

    code = *(addr + 1);
    addr += 2;

    if (code == 0xd9) {
      if (verbose) {
	fprintf(stderr,
		"   Found end of image after %ld bytes\n",
		(long) (addr - start + 1));
      }
      return addr - start;
    }

    if (code == 0x01 || code == 0xff) {
      if (verbose) {
	fprintf(stderr, "   Found lengthless section %02x\n", code);
      }
      continue;
    }

    size = (*addr << 8) + *(addr + 1);
    addr += size;

    if (verbose) {
      fprintf(stderr, "   Found section %02x of len %ld\n", code,
	      (long) size);
    }

    if (size < 2 || size > max_size) {
      if (verbose) {
	fprintf(stderr, "   Section size is out of bounds, aborting\n");
      }
      return 0;
    }

    if (code == 0xda) {
      if (verbose) {
	fprintf(stderr, "   Looking for end marker... ");
	fflush(stderr);
      }

      for (; (size_t) (addr - start) < max_size && (*addr != 0xff || *(addr + 1) == 0 ||	/* Escape */
						    (*(addr + 1) >= 0xd0
						     && *(addr + 1) <=
						     0xd7) /* RSTn */ );
	   addr++);

      if ((size_t) (addr - start) >= max_size) {
	if (verbose) {
	  fprintf(stderr, "too big, aborting\n");
	}
	return 0;
      }

      if (verbose) {
	fprintf(stderr, "found at offset %ld\n", (long) (addr - start));
      }

    }
  }
}

static const char *
file_name(const char *dir_format, const char *file_format, unsigned int index)
{
  static char dir_buffer[200];
  char file_buffer[100];

  if (dir_format) {
    snprintf(dir_buffer, sizeof dir_buffer, dir_format, index / 100);
    if (access(dir_buffer, F_OK) == -1 && mkdir(dir_buffer, 0777) == -1) {
      fprintf(stderr,
	      "recoverjpeg: unable to create directory %s (%s)\n",
	      dir_buffer, strerror(errno));
      exit(1);
    }
    strncat(dir_buffer, "/", sizeof dir_buffer);
  } else {
    *dir_buffer = '\0';
  }

  snprintf(file_buffer, sizeof file_buffer, file_format, index);
  strncat(dir_buffer, file_buffer, sizeof dir_buffer);
  return dir_buffer;
}

int
main(int argc, const char const *argv[])
{
  int fd, fdout;
  size_t read_size, block_size;
  unsigned int i, begin_index;
  unsigned char *start, *end, *addr;
  size_t size;
  int page_size;
  off_t offset;
  const char *file_format;
  const char *dir_format;
  int c;

  read_size = 128 * 1024 * 1024;
  block_size = 512;
  begin_index = 0;
  file_format = "image%05d.jpg";
  dir_format = NULL;

  while ((c = getopt(argc, (char * const *) argv, "b:d:f:hi:m:qr:s:vV")) != -1) {
    switch (c) {
    case 'b':
      block_size = atol_suffix(optarg);
      break;
    case 'd':
      dir_format = optarg;
      break;
    case 'f':
      file_format = optarg;
      break;
    case 'i':
      begin_index = atoi(optarg);
      break;
    case 'm':
      max_size = atol_suffix(optarg);
      break;
    case 'q':
      quiet = 1;
      break;
    case 'r':
      read_size = atol_suffix(optarg);
      break;
    case 's':
      ignore_size = atol_suffix(optarg) - 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'V':
      display_version_and_exit("recoverjpeg");
    default:
      usage(c == 'h');
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 1) {
    usage(0);
  }

  fd = open(argv[0], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr,
	    "recoverjpeg: unable to open %s for reading (%s)\n",
	    argv[argc - 1], strerror(errno));
    exit(1);
  }

  page_size = getpagesize();
  if (read_size % page_size || read_size < max_size) {
    if (read_size < max_size) {
      read_size = max_size;
    }
    read_size = (read_size + page_size - 1) / page_size * page_size;
    if (!quiet) {
      fprintf(stderr, "Adjusted read size to %ld bytes\n", (long) read_size);
    }
  }

  start = end = (unsigned char *) malloc(read_size);
  if (start == 0) {
    fprintf(stderr,
	    "recoverjpeg: cannot allocate necessary memory (%s)\n",
	    strerror(errno));
    exit(1);
  }

  for (i = 0, offset = 0, addr = NULL; addr < end;) {

    if (progressbar()) {
      display_progressbar(offset, i);
    }

    if (addr == NULL || (size_t) (start + read_size - addr) < max_size) {
      off_t base_offset;
      long n;

      base_offset = offset / page_size * page_size;

      lseek(fd, base_offset, SEEK_SET);
      n = read(fd, start, read_size);
      if (n < 0) {
	fprintf(stderr, "recoverjpeg: unable to read data (%s)\n",
		strerror(errno));
	exit(1);
      }
      end = start + n;
      addr = start + (offset - base_offset);
    }

    size = jpeg_size(addr);
    if (size > ignore_size) {
      size_t n;

      const char *buffer = file_name(dir_format, file_format, begin_index + i);
      i++;
      if (verbose) {
	printf("%s %ld bytes\n", buffer, (long) size);
      }
      fdout = open(buffer, O_WRONLY | O_CREAT, 0666);
      if (fdout < 0) {
	fprintf(stderr, "Unable to open %s for writing\n", buffer);
	exit(1);
      }
      if ((size_t) write(fdout, addr, size) != size) {
	fprintf(stderr, "Unable to write %ld bytes to %s\n",
		(long) size, buffer);
	exit(1);
      }
      close(fdout);

      n = ((size + block_size - 1) / block_size) * block_size;
      addr += n;
      offset += n;
    }
    else {
      addr += block_size;
      offset += block_size;
    }
  }

  if (progressbar()) {
    cleanup_progressbar();
  }

  if (!quiet) {
    printf("Restored %d picture%s\n", i, i > 1 ? "s" : "");
  }

  /* Free allocated memory to keep valgrind happy */
  free(start);

  exit(0);
}
