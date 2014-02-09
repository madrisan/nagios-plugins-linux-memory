#ifndef MEMINFO_H_
# define MEMINFO_H_

/* Linux */
#if HAVE_PROC_MEMINFO
# define MEM_DATATYPE unsigned long
# define SU(X) ( ((unsigned long long)(X) << 10) >> shift), units
# define UNIT "%Lu"
#endif

/* OpenBSD */
#if HAVE_OPENBSD_SYSCTL
# define MEM_DATATYPE int
# define SU(X) ( ((unsigned int)(X) << 10) >> shift), units
# define UNIT "%u"
#endif

extern MEM_DATATYPE kb_main_used;
extern MEM_DATATYPE kb_main_total;
extern MEM_DATATYPE kb_main_free;
#if HAVE_MEMORY_SHARED
extern MEM_DATATYPE kb_main_shared;
#endif
#if HAVE_MEMORY_BUFFERS
extern MEM_DATATYPE kb_main_buffers;
#endif
extern MEM_DATATYPE kb_main_cached;

extern MEM_DATATYPE kb_swap_used;
extern MEM_DATATYPE kb_swap_total;
extern MEM_DATATYPE kb_swap_free;

extern unsigned long kb_swap_pagesin;
extern unsigned long kb_swap_pagesout;

void meminfo (void);
void swapinfo (void);

#endif
