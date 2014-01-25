/*
 * License: GPLv2
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check memory or swap usage on linux
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

#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nputils.h"

static const char *program_name = "check_memory";
static const char *program_version = PACKAGE_VERSION;
static const char *program_copyright =
  "Copyright (C) 2014 Davide Madrisan <" PACKAGE_BUGREPORT ">";

static void __attribute__ ((__noreturn__)) usage (FILE * out)
{
  fprintf (out,
           "%s, version %s - check memory or swap usage.\n",
           program_name, program_version);
  fprintf (out, "%s\n\n", program_copyright);
  fprintf (out, "Usage: %s {-M|-S} [-b,-k,-m,-g] -w PERC -c PERC\n", program_name);
  fprintf (out, "       %s -h\n", program_name);
  fprintf (out, "       %s -V\n\n", program_name);
  fputs ("\
Options:\n\
  -M, --memory   show the memory usage\n\
  -S, --swap     show the swap usage\n\
  -b,-k,-m,-g    show output in bytes, KB (the default), MB, or GB\n\
  -w, --warning PERCENT   warning threshold\n\
  -c, --critical PERCENT   critical threshold\n\
  -h, --help            display this help and exit\n\
  -v, --version         output version information and exit\n\n", out);

  exit (out == stderr ? STATE_UNKNOWN : STATE_OK);
}

static void __attribute__ ((__noreturn__)) print_version (void)
{
  printf ("%s, version %s\n", program_name, program_version);
  printf ("%s\n", program_copyright);
  fputs ("\
License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n\n\
This is free software; you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n", stdout);

  exit (STATE_OK);
}

#define BAD_OPEN_MESSAGE \
"Error: /proc must be mounted\n"

#define MEMINFO_FILE "/proc/meminfo"
static int meminfo_fd = -1;

/* As of 2.6.24 /proc/meminfo seems to need 888 on 64-bit,
 * and would need 1258 if the obsolete fields were there.
 */
static char buf[2048];

/* This macro opens filename only if necessary and seeks to 0 so
 * that successive calls to the functions are more efficient.
 * It also reads the current contents of the file into the global buf.
 */
#define FILE_TO_BUF(filename, fd) do{                           \
    static int local_n;                                         \
    if (fd == -1 && (fd = open(filename, O_RDONLY)) == -1) {    \
        fputs(BAD_OPEN_MESSAGE, stdout);                        \
        fflush(NULL);                                           \
        exit(STATE_UNKNOWN);                                    \
    }                                                           \
    lseek(fd, 0L, SEEK_SET);                                    \
    if ((local_n = read(fd, buf, sizeof buf - 1)) < 0) {        \
        perror(filename);                                       \
        fflush(NULL);                                           \
        exit(STATE_UNKNOWN);                                    \
    }                                                           \
    buf[local_n] = '\0';                                        \
}while(0)

/* example data, following junk, with comments added:
 *
 * MemTotal:        61768 kB    old
 * MemFree:          1436 kB    old
 * MemShared:           0 kB    old (now always zero; not calculated)
 * Buffers:          1312 kB    old
 * Cached:          20932 kB    old
 * Active:          12464 kB    new
 * Inact_dirty:      7772 kB    new
 * Inact_clean:      2008 kB    new
 * Inact_target:        0 kB    new
 * Inact_laundry:       0 kB    new, and might be missing too
 * HighTotal:           0 kB
 * HighFree:            0 kB
 * LowTotal:        61768 kB
 * LowFree:          1436 kB
 * SwapTotal:      122580 kB    old
 * SwapFree:        60352 kB    old
 * Inactive:        20420 kB    2.5.41+
 * Dirty:               0 kB    2.5.41+
 * Writeback:           0 kB    2.5.41+
 * Mapped:           9792 kB    2.5.41+
 * Slab:             4564 kB    2.5.41+
 * Committed_AS:     8440 kB    2.5.41+
 * PageTables:        304 kB    2.5.41+
 * ReverseMaps:      5738       2.5.41+
 * SwapCached:          0 kB    2.5.??+
 * HugePages_Total:   220       2.5.??+
 * HugePages_Free:    138       2.5.??+
 * Hugepagesize:     4096 kB    2.5.??+
 */

/* obsolete */
unsigned long kb_main_shared;
/* old but still kicking -- the important stuff */
unsigned long kb_main_buffers;
unsigned long kb_main_cached;
unsigned long kb_main_free;
unsigned long kb_main_total;
unsigned long kb_swap_free;
unsigned long kb_swap_total;
/* recently introduced */
unsigned long kb_high_free;
unsigned long kb_high_total;
unsigned long kb_low_free;
unsigned long kb_low_total;
/* 2.4.xx era */
unsigned long kb_active;
unsigned long kb_inact_laundry;
unsigned long kb_inact_dirty;
unsigned long kb_inact_clean;
unsigned long kb_inact_target;
unsigned long kb_swap_cached;  /* late 2.4 and 2.6+ only */
/* derived values */
unsigned long kb_swap_used;
unsigned long kb_main_used;
/* 2.5.41+ */
unsigned long kb_writeback;
unsigned long kb_slab;
unsigned long nr_reversemaps;
unsigned long kb_committed_as;
unsigned long kb_dirty;
unsigned long kb_inactive;
unsigned long kb_mapped;
unsigned long kb_pagetables;
// seen on a 2.6.x kernel:
static unsigned long kb_vmalloc_chunk;
static unsigned long kb_vmalloc_total;
static unsigned long kb_vmalloc_used;
// seen on 2.6.24-rc6-git12
static unsigned long kb_anon_pages;
static unsigned long kb_bounce;
static unsigned long kb_commit_limit;
static unsigned long kb_nfs_unstable;
static unsigned long kb_swap_reclaimable;
static unsigned long kb_swap_unreclaimable;

typedef struct mem_table_struct {
  const char *name;     /* memory type name */
  unsigned long *slot; /* slot in return struct */
} mem_table_struct;

static int compare_mem_table_structs(const void *a, const void *b){
  return strcmp(((const mem_table_struct*)a)->name, ((const mem_table_struct*)b)->name);
}

void
meminfo (void)
{
  char namebuf[16];		/* big enough to hold any row name */
  mem_table_struct findme = { namebuf, NULL };
  mem_table_struct *found;
  char *head;
  char *tail;
  static const mem_table_struct mem_table[] = {
    { "Active",        &kb_active},             /* important */
    { "AnonPages",     &kb_anon_pages},
    { "Bounce",        &kb_bounce},
    { "Buffers",       &kb_main_buffers},       /* important */
    { "Cached",        &kb_main_cached},        /* important */
    { "CommitLimit",   &kb_commit_limit},
    { "Committed_AS",  &kb_committed_as},
    { "Dirty",         &kb_dirty},              /* kB version of vmstat nr_dirty */
    { "HighFree",      &kb_high_free},
    { "HighTotal",     &kb_high_total},
    { "Inact_clean",   &kb_inact_clean},
    { "Inact_dirty",   &kb_inact_dirty},
    { "Inact_laundry", &kb_inact_laundry},
    { "Inact_target",  &kb_inact_target},
    { "Inactive",      &kb_inactive},	        /* important */
    { "LowFree",       &kb_low_free},
    { "LowTotal",      &kb_low_total},
    { "Mapped",        &kb_mapped},             /* kB version of vmstat nr_mapped */
    { "MemFree",       &kb_main_free},	        /* important */
    { "MemShared",     &kb_main_shared},        /* important, but now gone! */
    { "MemTotal",      &kb_main_total},	        /* important */
    { "NFS_Unstable",  &kb_nfs_unstable},
    { "PageTables",    &kb_pagetables},	        /* kB version of vmstat nr_page_table_pages */
    { "ReverseMaps",   &nr_reversemaps},        /* same as vmstat nr_page_table_pages */
    { "SReclaimable",  &kb_swap_reclaimable},   /* "swap reclaimable" (dentry and inode structures) */
    { "SUnreclaim",    &kb_swap_unreclaimable},
    { "Slab",          &kb_slab},               /* kB version of vmstat nr_slab */
    { "SwapCached",    &kb_swap_cached},
    { "SwapFree",      &kb_swap_free},          /* important */
    { "SwapTotal",     &kb_swap_total},         /* important */
    { "VmallocChunk",  &kb_vmalloc_chunk},
    { "VmallocTotal",  &kb_vmalloc_total},
    { "VmallocUsed",   &kb_vmalloc_used},
    { "Writeback",     &kb_writeback},          /* kB version of vmstat nr_writeback */
  };
  const int mem_table_count = sizeof (mem_table) / sizeof (mem_table_struct);

  FILE_TO_BUF (MEMINFO_FILE, meminfo_fd);

  kb_inactive = ~0UL;

  head = buf;
  for (;;)
    {
      tail = strchr (head, ':');
      if (!tail)
	break;
      *tail = '\0';
      if (strlen (head) >= sizeof (namebuf))
	{
	  head = tail + 1;
	  goto nextline;
	}
      strcpy (namebuf, head);
      found = bsearch (&findme, mem_table, mem_table_count,
		       sizeof (mem_table_struct), compare_mem_table_structs);
      head = tail + 1;
      if (!found)
	goto nextline;
      *(found->slot) = strtoul (head, &tail, 10);

    nextline:
      tail = strchr (head, '\n');
      if (!tail)
	break;
      head = tail + 1;
    }

  if (!kb_low_total)
    {				/* low==main except with large-memory support */
      kb_low_total = kb_main_total;
      kb_low_free = kb_main_free;
    }

  if (kb_inactive == ~0UL)
    {
      kb_inactive = kb_inact_dirty + kb_inact_clean + kb_inact_laundry;
    }

  kb_swap_used = kb_swap_total - kb_swap_free;
  kb_main_used = kb_main_total - kb_main_free;
}

#define S(X) ( ((unsigned long long)(X) << 10) >> shift)

static struct option const longopts[] = {
  {(char *) "memory", no_argument, NULL, 'M'},
  {(char *) "swap", no_argument, NULL, 'S'},
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

#define BUFSIZE 127
static char result_line[BUFSIZE + 1];

int
main (int argc, char **argv)
{
  int c, status;
  int show_memory = 0, show_swap = 0;
  int shift = 10; /* output in kilobytes by default */
  char *critical = NULL, *warning = NULL;
  thresholds *my_threshold = NULL;
  float perc;

  while ((c = getopt_long (argc, argv, "MSc:w:bkmghV", longopts, NULL)) != -1)
    {
      switch (c)
        {
        default:
          usage (stderr);
          break;
        case 'M':
          show_memory = 1;
          break;
        case 'S':
          show_swap = 1;
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
        case 'b': shift = 0;  break;
        case 'k': shift = 10; break;
        case 'm': shift = 20; break;
        case 'g': shift = 30; break;
        }
    }

  if (show_memory + show_swap != 1)
    usage (stderr);

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);
  
  meminfo ();

  perc = show_memory ?
    (kb_main_used * 100.0 / kb_main_total) : (kb_swap_used * 100.0 / kb_swap_total);

  status = get_status (perc, my_threshold);
  switch (status)
    {
    case STATE_CRITICAL:
      c = snprintf (result_line, BUFSIZE, "CRITICAL:");
      break;
    case STATE_WARNING:
      c = snprintf (result_line, BUFSIZE, "WARNING:");
      break;
    case STATE_OK:
      c = snprintf (result_line, BUFSIZE, "OK:");
      break;
    }
  free (my_threshold);

  if (show_memory)
    {
      printf ("%s %.2f%% (%Lu kB) used | "
              "mem_total=%Lu, mem_used=%Lu, mem_free=%Lu, mem_shared=%Lu, mem_buffers=%Lu, mem_cached=%Lu\n",
              result_line, perc, kb_main_used,
              S (kb_main_total), S (kb_main_used), S (kb_main_free), S (kb_main_shared),
              S (kb_main_buffers), S (kb_main_cached));
    }
  else
    {
      printf
        ("%s %.2f%% (%Lu kB) used | swap_total=%Lu, swap_used=%Lu, swap_free=%Lu\n",
         result_line, perc, kb_swap_used, S (kb_swap_total), S (kb_swap_used),
         S (kb_swap_free));
    }

  return status;
}
