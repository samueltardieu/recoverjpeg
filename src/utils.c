/*
 * This file is part of the recoverjpeg program.
 *
 * Copyright (c) 2004-2016 Samuel Tardieu <sam@rfc1149.net>
 * http://www.rfc1149.net/devel/recoverjpeg
 *
 * recoverjpeg is released under the GNU General Public License
 * version 2 that you can find in the COPYING file bundled with the
 * distribution.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

typedef struct move_s
{
  const char *target;
  struct move_s *next;
} move_t;

static move_t *chdirs = NULL;

size_t
atol_suffix(char *arg)
{
  long multiplier = 1;

  switch (arg[strlen(arg) - 1]) {
  case 't':
  case 'T':
    multiplier *= 1024;
    /* Fallthrough */
  case 'g':
  case 'G':
    multiplier *= 1024;
    /* Fallthrough */
  case 'm':
  case 'M':
    multiplier *= 1024;
    /* Fallthrough */
  case 'k':
  case 'K':
    multiplier *= 1024;
  }

  if (multiplier != 1) {
    arg[strlen(arg) - 1] = '\0';
  }

  return atol(arg) * multiplier;
}

void
display_version_and_exit(const char *program_name)
{
  printf("%s %s (from the `%s' package)\n", program_name, VERSION, PACKAGE);
  exit(0);
}

void
record_chdir(const char *directory)
{
  move_t **ptr = &chdirs;
  while (*ptr != NULL) {
    ptr = &(*ptr)->next;
  }
  *ptr = malloc(sizeof(move_t));
  if (*ptr == NULL) {
    perror("malloc");
    exit(1);
  }
  (*ptr)->target = directory;
  (*ptr)->next = NULL;
}

void
perform_chdirs()
{
  move_t *to_free;
  move_t *p = chdirs;
  while (p != NULL) {
    if (chdir(p->target) != 0) {
      char buffer[512];
      snprintf(buffer, sizeof buffer, "cannot change directory to `%s'", p->target);
      perror(buffer);
      exit(1);
    }
    to_free = p;
    p = p->next;
    free(to_free);
  }
}
