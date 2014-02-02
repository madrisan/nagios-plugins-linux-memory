dnl Process this file with autoconf to produce a configure script.

m4_define(PROG_VERSION, 1)
m4_define(PROG_NAME, [nagios-plugins-memory])
m4_define(PROG_BUGREPORT, [davide.madrisan@gmail.com])

AC_INIT([PROG_NAME],[PROG_VERSION],[PROG_BUGREPORT],[PROG_NAME])
AC_PREREQ([2.59])

AC_CONFIG_SRCDIR([check_memory.c])
AC_CONFIG_HEADERS(config.h:config.hin)

AM_INIT_AUTOMAKE([gnu dist-bzip2])

AM_MAINTAINER_MODE
AC_CANONICAL_HOST

dnl Checks for programs
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL

dnl Checks for header files
AC_HEADER_STDC

AC_CHECK_HEADERS([sys/param.h])
# check sys/sysctl.h separately, as it requires other header
# on at least OpenBSD 
AC_CHECK_HEADERS([sys/sysctl.h], [], [],
[[#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
]])

AC_CHECK_HEADERS([sys/mount.h], [], [],
[[#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
]])

dnl Check for some target-specific stuff
case "$host" in

*-*-linux*)
  AC_DEFINE([HAVE_PROC_MEMINFO], [1],
    [Define to 1 if you have /proc/meminfo.])
  AC_DEFINE([HAVE_MEMORY_SHARED], [1],
    [Define to 1 if the os has the memory shared counter.])
  AC_DEFINE([HAVE_MEMORY_BUFFERS], [1],
    [Define to 1 if the os has the memory buffers counter.])
  ;;
*-*-openbsd*)
  AC_MSG_CHECKING(for function sysctl (VM_METER))
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
 
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
 
#include <sys/sysctl.h>
#include <unistd.h>
     ]],[[
int vmtotal_mib[] = {CTL_VM, VM_METER};
struct vmtotal vmtotal;
size_t size;

size = sizeof(vmtotal);
sysctl(vmtotal_mib, 2, &vmtotal, &size, NULL, 0);]])],
    [ac_cv_function_sysctl_vm_meter=yes
     AC_DEFINE_UNQUOTED(HAVE_FUNCTION_SYSCTL_VM_METER, 1,
       [Define to 1 if 'VM_METER' exists.])
    ],
    [ac_cv_function_sysctl_vm_meter=no])
  AC_MSG_RESULT([$ac_cv_function_sysctl_vm_meter])

  AC_MSG_CHECKING(for function sysctl (VFS_BCACHESTAT))
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#include <sys/mount.h>
#include <sys/sysctl.h>
#include <unistd.h>
     ]],[[
int bcstats_mib[] = {CTL_VFS, VFS_GENERIC, VFS_BCACHESTAT};
struct bcachestats bcstats;
size_t size;

size = sizeof(bcstats);
sysctl(bcstats_mib, 3, &bcstats, &size, NULL, 0);]])],
    [ac_cv_function_sysctl_vfs_bcachestat=yes
     AC_DEFINE_UNQUOTED(HAVE_FUNCTION_SYSCTL_VFS_BCACHESTAT, 1,
       [Define to 1 if 'VFS_BCACHESTAT' exists.])
    ],
    [ac_cv_function_sysctl_vfs_bcachestat=no])
AC_MSG_RESULT([$ac_cv_function_sysctl_vfs_bcachestat])

  if test x$ac_cv_function_sysctl_vm_meter = xyes && \
    test x$ac_cv_function_sysctl_vfs_bcachestat = xyes; then
      AC_DEFINE([HAVE_OPENBSD_SYSCTL], [1],
       [Define to 1 if you have all the required OpenBSD sysctl calls.])
  fi
  ;;
*)
  AC_MSG_ERROR("This Platform is not (yet) supported.")
  ;;
esac

AC_PREFIX_DEFAULT(/usr/local/nagios)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_PROTOTYPES

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
