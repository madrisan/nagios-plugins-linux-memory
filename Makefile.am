libexec_PROGRAMS = check_memory

check_memory_SOURCES = \
	check_memory.c \
	nputils.c nputils.h \
	meminfo.h

EXTRA_check_memory_SOURCES = \
	meminfo-linux.c \
	meminfo-openbsd.c

check_memory_LDADD = $(MEMINFO_MODULE)
check_memory_DEPENDENCIES = $(MEMINFO_MODULE)
