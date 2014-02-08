/*
 * License: GPLv2
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check memory usage on unix.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This software is based on the source code of the tool "free" (procps 3.2.8)
 */

#include "config.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nputils.h"
#include "meminfo.h"

static const char *program_name = "check_memory";
static const char *program_version = PACKAGE_VERSION;
static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">";

static void __ATTRIBUTE_NORETURN__ usage (FILE * out)
{
  fprintf (out,
           "%s, version %s - check memory usage.\n",
           program_name, program_version);
  fprintf (out, "%s\n\n", program_copyright);
  fprintf (out,
           "Usage: %s [-b,-k,-m,-g] [-C] -w PERC -c PERC\n",
           program_name);
  fprintf (out, "       %s -h\n", program_name);
  fprintf (out, "       %s -V\n\n", program_name);
  fputs ("\
Options:\n\
  -b,-k,-m,-g      show output in bytes, KB (the default), MB, or GB\n\
  -C, --caches     count buffers and cached memory as free memory\n\
  -w, --warning PERCENT   warning threshold\n\
  -c, --critical PERCENT   critical threshold\n\
  -h, --help       display this help and exit\n\
  -v, --version    output version information and exit\n\n", out);
  fprintf (out, "\
Examples:\n\
  %s -C -w 80%% -c90%%\n", program_name);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static void __ATTRIBUTE_NORETURN__ print_version (void)
{
  printf ("%s, version %s\n", program_name, program_version);
  printf ("%s\n", program_copyright);
  fputs ("\
License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n\n\
This is free software; you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n", stdout);

  exit (STATE_OK);
}

static struct option const longopts[] = {
  {(char *) "caches", no_argument, NULL, 'C'},
  {(char *) "critical", required_argument, NULL, 'c'},
  {(char *) "warning", required_argument, NULL, 'w'},
  {(char *) "byte", no_argument, NULL, 'b'},
  {(char *) "kilobyte", no_argument, NULL, 'k'},
  {(char *) "megabyte", no_argument, NULL, 'm'},
  {(char *) "gigabyte", no_argument, NULL, 'g'},
  {(char *) "help", no_argument, NULL, 'h'},
  {(char *) "version", no_argument, NULL, 'V'},
  {NULL, 0, NULL, 0}
};

int
main (int argc, char **argv)
{
  int c, status;
  int cache_is_free = 0;
  int shift = 10;
  char *critical = NULL, *warning = NULL;
  char *units = NULL;
  char statusbuf[10];                 /* big enough to hold the plugin status */
  thresholds *my_threshold = NULL;
  float perc;

  while ((c = getopt_long (argc, argv, "MSCc:w:bkmghV", longopts, NULL)) != -1)
    {
      switch (c)
        {
        default:
          usage (stderr);
          break;
        case 'C':
          cache_is_free = 1;
          break;
        case 'c':
          critical = optarg;
          break;
        case 'w':
          warning = optarg;
          break;
        case 'h':
          usage (stdout);
          break;
        case 'V':
          print_version ();
          break;
        case 'b': shift = 0;  units = strdup ("B"); break;
        case 'k': shift = 10; units = strdup ("kB"); break;
        case 'm': shift = 20; units = strdup ("MB"); break;
        case 'g': shift = 30; units = strdup ("GB"); break;
        }
    }

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  /* output in kilobytes by default */
  if (units == NULL)
    units = strdup ("kB");

  meminfo ();

  if (cache_is_free)
    {
#if HAVE_MEMORY_BUFFERS
      kb_main_used -= (kb_main_cached + kb_main_buffers);
      kb_main_free += (kb_main_cached + kb_main_buffers);
#else
      kb_main_used -= kb_main_cached;
      kb_main_free += kb_main_cached;
#endif
    }

  perc = (kb_main_used * 100.0 / kb_main_total);

  status = get_status (perc, my_threshold);
  switch (status)
    {
    case STATE_CRITICAL:
      c = sprintf (statusbuf, "CRITICAL:");
      break;
    case STATE_WARNING:
      c = sprintf (statusbuf, "WARNING:");
      break;
    case STATE_OK:
      c = sprintf (statusbuf, "OK:");
      break;
    }
  free (my_threshold);

  printf ("%s %.2f%% (" UNIT " %s) used | "
          "mem_total=" UNIT "%s, "
          "mem_used=" UNIT "%s, "
          "mem_free=" UNIT "%s, "
#if HAVE_MEMORY_SHARED
          "mem_shared=" UNIT "%s, "
#endif
#if HAVE_MEMORY_BUFFERS
          "mem_buffers=" UNIT "%s, "
#endif
          "mem_cached=" UNIT "%s\n",
          statusbuf, perc, SU (kb_main_used),
          SU (kb_main_total),
          SU (kb_main_used),
          SU (kb_main_free),
#if HAVE_MEMORY_SHARED
          SU (kb_main_shared),
#endif
#if HAVE_MEMORY_BUFFERS
          SU (kb_main_buffers),
#endif
          SU (kb_main_cached));

  free (units);

  return status;
}
