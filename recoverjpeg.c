/* Needed on Linux to work on large files or devices */

#define _FILE_OFFSET_BITS 64

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

static int verbose = 0;
static int quiet = 0;
static char size_buffer[10];

#define MAX_SIZE (6*1024*1024)
#define NPAGES (32*1024)

static inline int
progressbar ()
{
  return !(quiet || verbose);
}

static char *
make_size (off_t offset)
{
  if (offset < 1024*1024*1024) {
    snprintf (size_buffer, sizeof size_buffer,
	      "%4.1f MiB", (float) offset / (1024 * 1024));
  } else {
    snprintf (size_buffer, sizeof size_buffer,
	      "%4.1f GiB", (float) offset / (1024*1024*1024));
  }
  return size_buffer;
}

static void
display_progressbar (off_t offset, unsigned int n)
{
  printf ("\rRecovered files: %4u        Analyzed: %s", n, make_size (offset));
}

static size_t
jpeg_size (unsigned char *start)
{
  unsigned char *addr;
  unsigned char code;
  size_t size;

  if (*start != 0xff || *(start+1) != 0xd8) {
    return 0;
  }

  if (verbose) {
    fprintf (stderr, "Candidate jpeg found\n");
  }

  for (addr = start + 2;;) {
    if (*addr != 0xff) {
      if (verbose) {
	fprintf (stderr,
		 "   Incorrect marker %02x, stopping prematurely\n", *addr);
      }
      return 0;
    }

    code = *(addr+1);
    addr += 2;

    if (code == 0xd9) {
      if (verbose) {
	fprintf (stderr,
		 "   Found end of image after %d bytes\n", addr-start+1);
      }
      return addr-start;
    }

    if ((code >= 0xd0 && code <= 0xd9) || code == 0x01 || code == 0xff) {
      if (verbose) {
	fprintf (stderr,
		 "   Found lengthless section %02x\n", code);
      }
      continue;
    }

    size = (*addr << 8) + *(addr+1);
    addr += size;

    if (verbose) {
      fprintf (stderr, "   Found section %02x of len %d\n", code, size);
    }

    if (size < 2 || size > MAX_SIZE) {
      if (verbose) {
	fprintf (stderr, "   Section size is out of bounds, aborting\n");
      }
      return 0;
    }

    if (code == 0xda) {
      if (verbose) {
	fprintf (stderr, "   Looking for end marker... ");
	fflush (stderr);
      }

      for (;
	   addr-start < MAX_SIZE && (*addr != 0xff || *(addr+1) == 0);
	   addr++);

      if (addr - start >= MAX_SIZE) {
	if (verbose) {
	  fprintf (stderr, "too big, aborting\n");
	}
	return 0;
      }

      if (verbose) {
	fprintf (stderr, "found\n");
      }

    }
  }
}

int
main (int argc, char *argv[])
{
  int fd, fdout;
  unsigned int i;
  unsigned char *start, *addr;
  size_t size;
  char buffer[100];
  long page_size, mmap_size;
  off_t offset;

  offset = 0;
  start = addr = NULL;
  page_size = sysconf (_SC_PAGESIZE);
  mmap_size = NPAGES * page_size;

  for (i = 1; i < argc - 1; i++) {
     if (argv[i][0] != '-') {
      continue;
    }
    switch (argv[i][1]) {
    case 'v': verbose = 1; break;
    case 'q': quiet = 1; break;
    }
  }

  if (argc < 2) {
    fprintf (stderr, "Usage: recoverjpeg [-v|-q] device\n");
    exit (1);
  }

  fd = open (argv[argc-1], O_RDONLY);
  if (fd < 0) {
    fprintf (stderr, "Unable to open %s for reading\n", argv[argc-1]);
    exit (1);
  }

  /* Run forever, the program will end with an nmap error */
  for (i = 0, offset = 0;;) {

    if (progressbar ()) {
      display_progressbar (offset, i);
    }

    if (start == NULL || (start + mmap_size - addr) < MAX_SIZE) {
      off_t base_offset;
      base_offset = offset / page_size * page_size;

      if (start != NULL) {
	munmap (start, mmap_size);
      }
      start = mmap (NULL, mmap_size, PROT_READ, MAP_SHARED, fd, base_offset);
      if (start == MAP_FAILED) {
	perror ("Unable to map data");
	fprintf (stderr, "Arguments to mmap were: "
		 "mmap (NULL, %lu, PROT_READ, MAP_SHARED, %d, %lu)\n",
		 mmap_size, fd, base_offset);
	exit (1);
      }
      addr = start + (offset - base_offset);
    }

    size = jpeg_size (addr);
    if (size > 0) {
      snprintf (buffer, sizeof buffer, "image%05d.jpg", i++);
      if (verbose) {
	printf ("%s %d bytes\n", buffer, size);
      }
      fdout = open (buffer, O_WRONLY | O_CREAT, 0666);
      if (fdout < 0) {
	fprintf (stderr, "Unable to open %s for writing\n", buffer);
	exit (1);
      }
      if (write (fdout, addr, size) != size) {
	fprintf (stderr, "Unable to write %d bytes to %s\n", size, buffer);
	exit (1);
      }
      close (fdout);
      addr += ((size + 511) / 512) * 512;
      offset += ((size + 511) / 512) * 512;
    } else {
      addr += 512;
      offset += 512;
    }
  }

  exit (0);
}
