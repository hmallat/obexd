AC_PREREQ(2.60)
AC_INIT(obexd, 0.48)

AM_INIT_AUTOMAKE([foreign subdir-objects color-tests])
AM_CONFIG_HEADER(config.h)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT(/usr/local)

if (test "${libdir}" = '${exec_prefix}/lib'); then
	libdir="${prefix}/lib"
fi

plugindir="${libdir}/obex/plugins"

if (test "$sysconfdir" = '${prefix}/etc'); then
	configdir="${prefix}/etc/obex"
else
	configdir="${sysconfdir}/obex"
fi

AC_DEFINE_UNQUOTED(CONFIGDIR, "${configdir}",
				[Directory for the configuration files])

if (test "$USE_MAINTAINER_MODE" = "yes"); then
	AC_CHECK_PROG(enable_coverage, [gcov], [yes], [no])
fi
AM_CONDITIONAL(COVERAGE, test "${enable_coverage}" = "yes")

COMPILER_FLAGS

AC_LANG_C

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_PIE
AC_PROG_INSTALL
AC_PROG_SED

AC_PATH_READLINE

m4_define([_LT_AC_TAGCONFIG], [])
m4_ifdef([AC_LIBTOOL_TAGS], [AC_LIBTOOL_TAGS([])])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_CACHE_CHECK(for tm_gmtoff in struct tm, ac_cv_struct_tm_gmtoff,
	AC_TRY_COMPILE([
		#include <time.h>
	], [
		struct tm tm;
		tm.tm_gmtoff = 1;
	], ac_cv_struct_tm_gmtoff=yes, ac_cv_struct_tm_gmtoff=no))

if (test "$ac_cv_struct_tm_gmtoff" = "yes"); then
	AC_DEFINE(HAVE_TM_GMTOFF, 1, [Define if struct tm has a tm_gmtoff member])
fi

AC_CACHE_CHECK(for timezone variable, ac_cv_var_timezone,
	AC_TRY_COMPILE([
		#include <time.h>
	], [
		timezone = 1;
	], ac_cv_var_timezone=yes, ac_cv_var_timezone=no))

if (test "$ac_cv_var_timezone" = "yes"); then
	AC_DEFINE(HAVE_TIMEZONE, 1, [Define if libc defines a timezone variable])
fi

AC_CHECK_LIB(dl, dlopen, dummy=yes,
			AC_MSG_ERROR(dynamic linking loader is required))

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.28, dummy=yes,
				AC_MSG_ERROR(GLib >= 2.28 is required))
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.4, dummy=yes,
				AC_MSG_ERROR(D-Bus >= 1.4 is required))
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(BLUEZ, bluez >= 4.100, dummy=yes,
				AC_MSG_ERROR(BlueZ >= 4.100 is required))
AC_SUBST(BLUEZ_CFLAGS)
AC_SUBST(BLUEZ_LIBS)

AC_ARG_ENABLE(optimization, AC_HELP_STRING([--disable-optimization],
			[disable code optimization through compiler]), [
	if (test "${enableval}" = "no"); then
		CFLAGS="$CFLAGS -O0"
	fi
])

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug],
			[enable compiling with debugging information]), [
	if (test "${enableval}" = "yes" && test "${ac_cv_prog_cc_g}" = "yes"); then
		CFLAGS="$CFLAGS -g"
	fi
])

messages_driver=dummy
AC_ARG_WITH(messages, AC_HELP_STRING([--with-messages=DRIVER], [select messages driver]), [
	if (test "${withval}" = "no"); then
		messages_driver=dummy;
	else
		messages_driver=${withval};
	fi
])

AC_SUBST([MESSAGES_DRIVER], [messages-${messages_driver}.c])

phonebook_driver=dummy
AC_ARG_WITH(phonebook, AC_HELP_STRING([--with-phonebook=DRIVER], [select phonebook driver]), [
	if (test "${withval}" = "no"); then
		phonebook_driver=dummy;
	else
		phonebook_driver=${withval};
	fi
])

if (test "${phonebook_driver}" = "dummy"); then
	PKG_CHECK_MODULES(LIBICAL, libical, dummy=yes,
					AC_MSG_ERROR(libical is required))
	AC_SUBST(LIBICAL_CFLAGS)
	AC_SUBST(LIBICAL_LIBS)
fi

if (test "${phonebook_driver}" = "ebook"); then
	AC_DEFINE(NEED_THREADS, 1, [Define if threading support is required])

	PKG_CHECK_MODULES(EBOOK, libebook-1.2, dummy=yes,
					AC_MSG_ERROR(libebook is required))
	AC_SUBST(EBOOK_CFLAGS)
	AC_SUBST(EBOOK_LIBS)

	PKG_CHECK_MODULES(GTHREAD, gthread-2.0, dummy=yes,
					AC_MSG_ERROR(libgthread is required))
	AC_SUBST(GTHREAD_CFLAGS)
	AC_SUBST(GTHREAD_LIBS)
fi

if (test "${phonebook_driver}" = "tracker"); then
	PKG_CHECK_MODULES(TRACKER_09, tracker-sparql-0.9, [
			TRACKER_CFLAGS=${TRACKER_09_CFLAGS}
			TRACKER_LIBS=${TRACKER_09_LIBS}
		], [
			PKG_CHECK_MODULES(TRACKER_10, tracker-sparql-0.10, [
					TRACKER_CFLAGS=${TRACKER_10_CFLAGS}
					TRACKER_LIBS=${TRACKER_10_LIBS}
			], AC_MSG_ERROR(libtracker-sparql is required))])
	AC_SUBST(TRACKER_CFLAGS)
	AC_SUBST(TRACKER_LIBS)
fi

AC_SUBST([PHONEBOOK_DRIVER], [phonebook-${phonebook_driver}.c])

contentfilter_driver=dummy
AC_ARG_WITH(contentfilter, AC_HELP_STRING([--with-contentfilter=DRIVER], [select content filter driver]), [
	if (test "${withval}" = "no"); then
		contentfilter_driver=dummy;
	else
		contentfilter_driver=${withval};
	fi
])

AC_SUBST([CONTENTFILTER_DRIVER], [contentfilter-${contentfilter_driver}.c])

AC_ARG_ENABLE(usb, AC_HELP_STRING([--enable-usb],
				[enable USB plugin]), [
	enable_usb=${enableval}
])
AM_CONDITIONAL(USB, test "${enable_usb}" = "yes")

AC_ARG_ENABLE(pcsuite, AC_HELP_STRING([--enable-pcsuite],
				[enable Nokia PcSuite plugin]), [
	enable_pcsuite=${enableval}
])
AM_CONDITIONAL(NOKIA_PCSUITE, test "${enable_pcsuite}" = "yes")

AC_ARG_ENABLE(server, AC_HELP_STRING([--disable-server],
				[disable compilation of OBEX server]), [
	enable_server=${enableval}
])
AM_CONDITIONAL(SERVER, test "${enable_server}" != "no")

AC_ARG_ENABLE(client, AC_HELP_STRING([--disable-client],
				[disable compilation of OBEX client]), [
	enable_client=${enableval}
])
AM_CONDITIONAL(CLIENT, test "${enable_client}" != "no")

AM_CONDITIONAL(READLINE, test "${readline_found}" = "yes")

AC_ARG_ENABLE(jolla-blacklist, AC_HELP_STRING([--enable-jolla-blacklist],
				[enable jolla-blacklist plugin]), [
	enable_jolla_blacklist=${enableval}
])
AM_CONDITIONAL(JOLLA_BLACKLIST, test "${enable_jolla_blacklist}" = "yes")

AC_OUTPUT(Makefile)
