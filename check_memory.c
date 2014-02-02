/*
 * License: GPLv2
 * Copyright (c) 2014 Davide Madrisan <davide.madrisan@gmail.com>
 *
 * A Nagios plugin to check memory and swap usage on linux
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

#if HAVE_OPENBSD_SYSCTL
# include <sys/param.h>
# include <sys/types.h>
# include <sys/mount.h>
# include <sys/sysctl.h>
# include <sys/swap.h>
# include <unistd.h>    /* getpagesize */
#endif

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
           "%s, version %s - check memory and swap usage.\n",
           program_name, program_version);
  fprintf (out, "%s\n\n", program_copyright);
  fprintf (out,
           "Usage: %s {-M|-S} [-C] [-b,-k,-m,-g] -w PERC -c PERC\n",
           program_name);
  fprintf (out, "       %s -h\n", program_name);
  fprintf (out, "       %s -V\n\n", program_name);
  fputs ("\
Options:\n\
  -M, --memory     show the memory usage\n\
  -S, --swap       show the swap usage\n\
  -b,-k,-m,-g      show output in bytes, KB (the default), MB, or GB\n\
  -C, --caches     count buffers and cached memory as free memory\n\
  -w, --warning PERCENT   warning threshold\n\
  -c, --critical PERCENT   critical threshold\n\
  -h, --help       display this help and exit\n\
  -v, --version    output version information and exit\n\n", out);
  fprintf (out, "\
Examples:\n\
  %s --memory -C -w 80%% -c90%%\n", program_name);
  fprintf (out, "  %s --swap -w 30%% -c 50%%\n\n", program_name);

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

#if HAVE_PROC_MEMINFO
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
        fputs("Error: /proc must be mounted\n", stdout);        \
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

#define SU(X) ( ((unsigned long long)(X) << 10) >> shift), units

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

  FILE_TO_BUF ("/proc/meminfo", meminfo_fd);

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
#endif   /* HAVE_PROC_MEMINFO */


#if HAVE_OPENBSD_SYSCTL
# define NUM_AVERAGES    3
  /* Log base 2 of 1024 is 10 (2^10 == 1024) */
# define LOG1024         10

/* these are for getting the memory statistics */
static int      pageshift;      /* log base 2 of the pagesize */

/* define pagetok in terms of pageshift */
#define pagetok(size) ((size) << pageshift)

#define SU(X) ( ((unsigned int)(X) << 10) >> shift), units

static int kb_main_used;
static int kb_main_cached;
static int kb_main_free;
static int kb_main_total;
static int kb_swap_used;
static int kb_swap_free;
static int kb_swap_total;

int
get_system_pagesize()
{
  int pagesize;

  /*
   * get the page size with "getpagesize" and calculate pageshift from
   * it
   * fixme: we should use sysconf(_SC_PAGESIZE) instead
   */
  pagesize = getpagesize ();
  pageshift = 0;
  while (pagesize > 1)
    {
      pageshift++;
      pagesize >>= 1;
    }

  /* we only need the amount of log(2)1024 for our conversion */
  pageshift -= LOG1024;

  return (0);
}

int
swapmode (int *used, int *total)
{
  struct swapent *swdev;
  int nswap, rnswap, i;

  nswap = swapctl (SWAP_NSWAP, 0, 0);
  if (nswap == 0)
    return 0;

  swdev = calloc (nswap, sizeof (*swdev));
  if (swdev == NULL)
    return 0;

  rnswap = swapctl (SWAP_STATS, swdev, nswap);
  if (rnswap == -1)
    {
      free (swdev);
      return 0;
    }

  /* if rnswap != nswap, then what? */

  /* Total things up */
  *total = *used = 0;
  for (i = 0; i < nswap; i++)
    {
      if (swdev[i].se_flags & SWF_ENABLE)
        {
          *used += (swdev[i].se_inuse / (1024 / DEV_BSIZE));
          *total += (swdev[i].se_nblks / (1024 / DEV_BSIZE));
        }
    }
  free (swdev);
  return 1;
}

void
meminfo (void)
{
  static int vmtotal_mib[] = { CTL_VM, VM_METER };
  static int bcstats_mib[] = { CTL_VFS, VFS_GENERIC, VFS_BCACHESTAT };
  struct vmtotal vmtotal;
  struct bcachestats bcstats;
  size_t size;

  if (get_system_pagesize() == -1)
    { 
      fputs("RUNTIME ERROR: get_system_pagesize failed\n", stdout);
      exit(STATE_UNKNOWN);
    }

  /* get total -- systemwide main memory usage structure */
  size = sizeof (vmtotal);
  if (sysctl (vmtotal_mib, 2, &vmtotal, &size, NULL, 0) < 0)
    { 
      bzero (&vmtotal, sizeof (vmtotal));
      fputs("RUNTIME ERROR: sysctl failed\n", stdout);
      exit(STATE_UNKNOWN);
    } 
  size = sizeof (bcstats);
  if (sysctl (bcstats_mib, 3, &bcstats, &size, NULL, 0) < 0)
    { 
      fputs("RUNTIME ERROR: sysctl failed\n", stdout);
      bzero (&bcstats, sizeof (bcstats));
      exit(STATE_UNKNOWN);
    } 

  /* convert memory stats to Kbytes */
  kb_main_total = pagetok (vmtotal.t_rm);
  kb_main_used = pagetok (vmtotal.t_arm);
  kb_main_free = pagetok (vmtotal.t_free);;
  kb_main_cached = pagetok (bcstats.numbufpages);

  if (!swapmode (&kb_swap_used, &kb_swap_total))
    {
      kb_swap_total = 0;
      kb_swap_used = 0;
    }
  kb_swap_free = kb_swap_total - kb_swap_used;
}
#endif   /* HAVE_OPENBSD_SYSCTL */


static struct option const longopts[] = {
  {(char *) "memory", no_argument, NULL, 'M'},
  {(char *) "swap", no_argument, NULL, 'S'},
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
  int show_memory = 0, show_swap = 0, cache_is_free = 0;
  int shift;
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
        case 'M':
          show_memory = 1;
          break;
        case 'S':
          show_swap = 1;
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

  if (show_memory + show_swap != 1)
    usage (stderr);

  status = set_thresholds (&my_threshold, warning, critical);
  if (status == NP_RANGE_UNPARSEABLE)
    usage (stderr);

  /* output in kilobytes by default */
  if (units == NULL)
    {
      units = strdup ("kB");
      shift = 10;
    }

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

  perc = show_memory ?
    (kb_main_used * 100.0 / kb_main_total) : (kb_swap_used * 100.0 / kb_swap_total);

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

  if (show_memory)
    {
      printf ("%s %.2f%% (%Lu %s) used | "
              "mem_total=%Lu%s, mem_used=%Lu%s, mem_free=%Lu%s, "
#if HAVE_MEMORY_SHARED
              "mem_shared=%Lu%s, "
#endif
#if HAVE_MEMORY_BUFFERS
              "mem_buffers=%Lu%s, "
#endif
              "mem_cached=%Lu%s\n",
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
              SU (kb_main_cached)
      );
    }
  else
    {
      printf
        ("%s %.2f%% (%Lu %s) used | "
         "swap_total=%Lu%s, swap_used=%Lu%s, swap_free=%Lu%s\n",
         statusbuf, perc, SU (kb_swap_used),
         SU (kb_swap_total),
         SU (kb_swap_used),
         SU (kb_swap_free)
      );
    }
  free (units);

  return status;
}
