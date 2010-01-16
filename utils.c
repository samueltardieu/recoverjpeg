/*
 * This file is part of the recoverjpeg program.
 *
 * Copyright (c) 2004-2010 Samuel Tardieu <sam@rfc1149.net>
 * http://www.rfc1149.net/devel/recoverjpeg
 *
 * recoverjpeg is released under the GNU General Public License
 * version 2 that you can find in the COPYING file bundled with the
 * distribution.
 */

#include <stdlib.h>
#include <string.h>
#include "utils.h"

size_t
atol_suffix (char *arg)
{
  long multiplier = 1;

  switch (arg[strlen(arg)-1]) {
  case 'g':
  case 'G':
    multiplier = 1024*1024*1024;
    break;
  case 'm':
  case 'M':
    multiplier = 1024*1024;
    break;
  case 'k':
  case 'K':
    multiplier = 1024;
    break;
  }

  if (multiplier != 1) {
    arg[strlen(arg)-1] = '\0';
  }

  return atol(arg) * multiplier;
}
