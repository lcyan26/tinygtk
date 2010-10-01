/* config.h.win32.in. Merged from two versions generated by configure for gcc and MSVC.  */
/* config.h.in.  Generated from configure.in by autoheader.  */
/* acconfig.h
   This file is in the public domain.

   Descriptive text for the C preprocessor macros that
   the distributed Autoconf macros can define.
   No software package will use all of them; autoheader copies the ones
   your configure.in uses into your configuration header file templates.

   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  Although this order
   can split up related entries, it makes it easier to check whether
   a given entry is in the file.

   Leave the following blank line there!!  Autoheader needs it.  */


/* Other stuff */
//#define ENABLE_NLS 1
#define GTK_COMPILED_WITH_DEBUGGING "minimal"

/* #undef HAVE_CATGETS */
/* #undef HAVE_DIMM_H */
//#define HAVE_GETTEXT 1
/* #undef HAVE_IPC_H */
/* #undef HAVE_LC_MESSAGES */
#define HAVE_PROGRESSIVE_JPEG 1
/* #undef HAVE_PWD_H */
/* #undef HAVE_SHM_H */
/* #undef HAVE_STPCPY */
/* #undef HAVE_XSHM_H */
/* #undef HAVE_SHAPE_EXT */
/* #undef HAVE_SYS_SELECT_H */
#ifndef _MSC_VER
#define HAVE_SYS_TIME_H 1
#else /* _MSC_VER */
/* #undef HAVE_SYS_TIME_H */
#endif /* _MSC_VER */
#define HAVE_WINSOCK_H 1
//#define HAVE_WINTAB 1
/* #undef HAVE_XCONVERTCASE */
/* #undef HAVE_XFT */

/* #undef HAVE_SIGSETJMP */

#define NO_FD_SET 1

/* #undef RESOURCE_BASE */

#ifndef _MSC_VER
#define USE_GMODULE 1
#define USE_MMX 1
#endif

/* Define to use X11R6 additions to XIM */
/* #undef USE_X11R6_XIM */

/* Define to use XKB extension */
/* #undef HAVE_XKB */

/* Define to use shadowfb in the linux-fb port */
/* #undef ENABLE_SHADOW_FB */

/* Define to use a fb manager in the linux-fb port */
/* #undef ENABLE_FB_MANAGER */

/* #undef XINPUT_NONE */
/* #undef XINPUT_GXI */
/* #undef XINPUT_XFREE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Most machines will be happy with int or void.  IRIX requires '...' */
/* #undef SIGNAL_ARG_TYPE */

#define GETTEXT_PACKAGE "gtk20"

/* #undef PACKAGE */
/* #undef VERSION */


/* Leave that blank line there!!  Autoheader needs it.
   If you're adding to this file, keep in mind:
   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  */

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define if using `alloca.c'. */
/* #undef C_ALLOCA */

/* always defined to indicate that i18n is enabled */
//#define ENABLE_NLS 1

/* Define if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix). */
/* #undef HAVE_ALLOCA_H */

/* Define if you have the <argz.h> header file. */
/* #undef HAVE_ARGZ_H */

/* Define if you have the `bind_textdomain_codeset' function. */
/* #undef HAVE_BIND_TEXTDOMAIN_CODESET */

/* Is the wctype implementation broken */
/* #undef HAVE_BROKEN_WCTYPE */

/* Define if you have the `dcgettext' function. */
//#define HAVE_DCGETTEXT 1

/* Define if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define if you have the `getpagesize' function. */
#ifndef _MSC_VER
#define HAVE_GETPAGESIZE 1
#else /* _MSC_VER */
/* #undef HAVE_GETPAGESIZE */
#endif /* _MSC_VER */

/* Define if you have the `getresuid' function. */
/* #undef HAVE_GETRESUID */

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define if your <locale.h> file defines LC_MESSAGES. */
/* #undef HAVE_LC_MESSAGES */

/* Define if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define if you have the `lstat' function. */
/* #undef HAVE_LSTAT */

/* Define if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if you have the `mkstemp' function. */
/* #undef HAVE_MKSTEMP */

/* Define if you have a working `mmap' system call. */
/* #undef HAVE_MMAP */

/* Define if you have the `munmap' function. */
/* #undef HAVE_MUNMAP */

/* Define if you have the <nl_types.h> header file. */
/* #undef HAVE_NL_TYPES_H */

/* Define if you have the `putenv' function. */
#define HAVE_PUTENV 1

/* Define if you have the <pwd.h> header file. */
/* #undef HAVE_PWD_H */

/* Define if you have the `setenv' function. */
/* #undef HAVE_SETENV */

/* Define if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define if you have the <stdint.h> header file. */
#ifndef _MSC_VER
#define HAVE_STDINT_H 1
#else /* _MSC_VER */
/* #undef HAVE_STDINT_H */
#endif /* _MSC_VER */

/* Define if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define if you have the `strcasecmp' function. */
#ifndef _MSC_VER
#define HAVE_STRCASECMP 1
#else /* _MSC_VER */
/* #undef HAVE_STRCASECMP */
#endif /* _MSC_VER */

/* Define if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define if you have the <sys/param.h> header file. */
/* #undef HAVE_SYS_PARAM_H */

/* Define if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file. */
#ifndef _MSC_VER
#define HAVE_SYS_TIME_H 1
#else /* _MSC_VER */
/* #undef HAVE_SYS_TIME_H */
#endif /* _MSC_VER */

/* Define if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible. */
/* #undef HAVE_SYS_WAIT_H */

/* Define if you have the <unistd.h> header file. */
#ifndef _MSC_VER
#define HAVE_UNISTD_H 1
#else /* _MSC_VER */
/* #undef HAVE_UNISTD_H */
#endif /* _MSC_VER */

/* Have wchar.h include file */
#define HAVE_WCHAR_H 1

/* Have wctype.h include file */
#define HAVE_WCTYPE_H 1

/* Define if you have the <winsock.h> header file. */
#define HAVE_WINSOCK_H 1

/* Define if you have the `__argz_count' function. */
/* #undef HAVE___ARGZ_COUNT */

/* Define if you have the `__argz_next' function. */
/* #undef HAVE___ARGZ_NEXT */

/* Define if you have the `__argz_stringify' function. */
/* #undef HAVE___ARGZ_STRINGIFY */

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
        STACK_DIRECTION > 0 => grows toward higher addresses
        STACK_DIRECTION < 0 => grows toward lower addresses
        STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define. */
#define gid_t int

/* Define as `__inline' if that's what the C compiler calls it, or to nothing
   if it is not supported. */
/* #undef inline */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> doesn't define. */
#define uid_t int
