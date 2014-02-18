#ifndef MEMINFO_H_
# define MEMINFO_H_

#include "config.h"

extern MEM_DATATYPE kb_main_used;
extern MEM_DATATYPE kb_main_total;
extern MEM_DATATYPE kb_swap_used;
extern MEM_DATATYPE kb_swap_total;

void meminfo (int);
void swapinfo (void);

char *get_memory_status (int, float, int, const char*);
char *get_memory_perfdata (int, const char*);

char *get_swap_status (int, float, int, const char*);
char *get_swap_perfdata (int, const char*);

#endif
