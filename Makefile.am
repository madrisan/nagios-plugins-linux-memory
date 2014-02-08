libexec_PROGRAMS = check_memory check_swap

check_memory_SOURCES = \
	check_memory.c \
	nputils.c nputils.h \
	meminfo.h
EXTRA_check_memory_SOURCES = \
	meminfo-linux.c \
	meminfo-openbsd.c
check_memory_LDADD = $(MEMINFO_MODULE)
check_memory_DEPENDENCIES = $(MEMINFO_MODULE)

check_swap_SOURCES = \
        check_swap.c \
        nputils.c nputils.h \
        meminfo.h
EXTRA_check_swap_SOURCES = \
        meminfo-linux.c \
        meminfo-openbsd.c
check_swap_LDADD = $(MEMINFO_MODULE)
check_swap_DEPENDENCIES = $(MEMINFO_MODULE)
