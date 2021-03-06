# BlueALSA - configure.ac
# Copyright (c) 2016-2019 Arkadiusz Bokowy

AC_PREREQ([2.60])
AC_INIT([bluez-alsa], [2.0.0], [arkadiusz.bokowy@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AR
AM_PROG_CC_C_O
LT_INIT

# configuration should match ALSA library
AC_PREFIX_DEFAULT([/usr])

# testing presence of pkg-config
AC_MSG_CHECKING([pkg-config m4 macros])
if test m4_ifdef([PKG_CHECK_MODULES], [yes], [no]) = "yes"; then
	AC_MSG_RESULT([yes]);
else
	AC_MSG_RESULT([no]);
	AC_MSG_ERROR([pkg-config is required. See pkg-config.freedesktop.org])
fi

# support for debugging
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [enable debugging support]))
AM_CONDITIONAL([ENABLE_DEBUG], [test "x$enable_debug" = "xyes"])
AM_COND_IF([ENABLE_DEBUG], [
	AC_DEFINE([DEBUG], [1], [Define to 1 if the debugging is enabled.])
])
AC_ARG_ENABLE([debug-time],
	AS_HELP_STRING([--enable-debug-time], [enable debug timing support]))
AM_CONDITIONAL([ENABLE_DEBUG_TIME], [test "x$enable_debug_time" = "xyes"])
AM_COND_IF([ENABLE_DEBUG_TIME], [
	AC_DEFINE([DEBUG_TIME], [1], [Define to 1 if the debug timing is enabled.])
])

AC_CHECK_FUNCS([splice],
	[], [AC_MSG_ERROR([unable to find splice() function])])
AC_SEARCH_LIBS([clock_gettime], [rt],
	[], [AC_MSG_ERROR([unable to find clock_gettime() function])])
AC_SEARCH_LIBS([pow], [m],
	[], [AC_MSG_ERROR([unable to find pow() function])])
AC_SEARCH_LIBS([pthread_create], [pthread],
	[], [AC_MSG_ERROR([pthread library not found])])

PKG_CHECK_MODULES([ALSA], [alsa])
PKG_CHECK_MODULES([BLUEZ], [bluez >= 5.0])
PKG_CHECK_MODULES([DBUS1], [dbus-1 >= 1.6])
PKG_CHECK_MODULES([GLIB2], [glib-2.0 >= 2.30])
PKG_CHECK_MODULES([GIO2], [gio-unix-2.0])
PKG_CHECK_MODULES([SBC], [sbc >= 1.2])

AM_CONDITIONAL([ALSA_1_1_2], [$PKG_CONFIG --atleast-version=1.1.2 alsa])
AM_CONDITIONAL([ALSA_1_1_7], [$PKG_CONFIG --atleast-version=1.1.7 alsa])

AC_ARG_ENABLE([aac],
	[AS_HELP_STRING([--enable-aac], [enable AAC support])])
AM_CONDITIONAL([ENABLE_AAC], [test "x$enable_aac" = "xyes"])
AM_COND_IF([ENABLE_AAC], [
	PKG_CHECK_MODULES([AAC], [fdk-aac >= 0.1.1])
	AC_DEFINE([ENABLE_AAC], [1], [Define to 1 if AAC is enabled.])
])

AC_ARG_ENABLE([aptx],
	[AS_HELP_STRING([--enable-aptx], [enable apt-X encoding support])])
AM_CONDITIONAL([ENABLE_APTX], [test "x$enable_aptx" = "xyes"])
AM_COND_IF([ENABLE_APTX], [
	PKG_CHECK_MODULES([APTX], [openaptx >= 1.0.0])
	AC_DEFINE([ENABLE_APTX], [1], [Define to 1 if apt-X is enabled.])
])

AC_ARG_ENABLE([aptx_hd],
	[AS_HELP_STRING([--enable-aptx-hd], [enable apt-X HD encoding support])])
AM_CONDITIONAL([ENABLE_APTX_HD], [test "x$enable_aptx_hd" = "xyes"])
AM_COND_IF([ENABLE_APTX_HD], [
	PKG_CHECK_MODULES([APTX_HD], [openaptxhd >= 1.0.0])
	AC_DEFINE([ENABLE_APTX_HD], [1], [Define to 1 if apt-X HD is enabled.])
])

AC_ARG_ENABLE([ldac],
	[AS_HELP_STRING([--enable-ldac], [enable LDAC encoding support])])
AM_CONDITIONAL([ENABLE_LDAC], [test "x$enable_ldac" = "xyes"])
AM_COND_IF([ENABLE_LDAC], [
	PKG_CHECK_MODULES([LDAC], [ldacBT-enc >= 2.0.0])
	PKG_CHECK_MODULES([LDAC_ABR], [ldacBT-abr >= 2.0.0])
	AC_DEFINE([ENABLE_LDAC], [1], [Define to 1 if LDAC is enabled.])
])

AC_ARG_ENABLE([mp3lame],
	[AS_HELP_STRING([--enable-mp3lame], [enable MP3 support])])
AM_CONDITIONAL([ENABLE_MP3LAME], [test "x$enable_mp3lame" = "xyes"])
AM_COND_IF([ENABLE_MP3LAME], [
	AC_CHECK_LIB([mp3lame], [lame_init],
		[AC_SUBST([MP3LAME_LIBS], [-lmp3lame])], [AC_MSG_ERROR([mp3lame library not found])])
	AC_DEFINE([ENABLE_MPEG], [1], [Define to 1 if MPEG is enabled.])
	AC_DEFINE([ENABLE_MP3LAME], [1], [Define to 1 if MP3LAME is enabled.])
])

AC_ARG_ENABLE([mpg123],
	[AS_HELP_STRING([--enable-mpg123], [enable MPG123 decoding support])])
AM_CONDITIONAL([ENABLE_MPG123], [test "x$enable_mpg123" = "xyes"])
AM_COND_IF([ENABLE_MPG123], [
	PKG_CHECK_MODULES([MPG123], [libmpg123 >= 1.0.0])
	AC_DEFINE([ENABLE_MPEG], [1], [Define to 1 if MPEG is enabled.])
	AC_DEFINE([ENABLE_MPG123], [1], [Define to 1 if MPG123 is enabled.])
])

AC_ARG_ENABLE([msbc],
	[AS_HELP_STRING([--enable-msbc], [enable mSBC support])])
AM_CONDITIONAL([ENABLE_MSBC], [test "x$enable_msbc" = "xyes"])
AM_COND_IF([ENABLE_MSBC], [
	AC_DEFINE([ENABLE_MSBC], [1], [Define to 1 if mSBC is enabled.])
])

AC_ARG_ENABLE([ofono],
	AS_HELP_STRING([--enable-ofono], [enable HFP over oFono]))
AM_CONDITIONAL([ENABLE_OFONO], [test "x$enable_ofono" = "xyes"])
AM_COND_IF([ENABLE_OFONO], [
	AC_DEFINE([ENABLE_OFONO], [1], [Define to 1 if oFono is enabled.])
])

AC_ARG_ENABLE([upower],
	AS_HELP_STRING([--enable-upower], [enable UPower integration]))
AM_CONDITIONAL([ENABLE_UPOWER], [test "x$enable_upower" = "xyes"])
AM_COND_IF([ENABLE_UPOWER], [
	AC_DEFINE([ENABLE_UPOWER], [1], [Define to 1 if UPower is enabled.])
])

AC_ARG_ENABLE([payloadcheck],
	[AS_HELP_STRING([--disable-payloadcheck], [disable RTP payload type check (workaround for a PulseAudio bug)])])
AM_CONDITIONAL([ENABLE_PAYLOADCHECK], [test "x$enable_payloadcheck" != "xno"])
AM_COND_IF([ENABLE_PAYLOADCHECK], [
	AC_DEFINE([ENABLE_PAYLOADCHECK], [1], [Define to 1 if PAYLOADCHECK is enabled.])
])

AC_ARG_ENABLE([aplay],
	[AS_HELP_STRING([--disable-aplay], [disable building of bluealsa-aplay tool])])
AM_CONDITIONAL([ENABLE_APLAY], [test "x$enable_aplay" != "xno"])

AC_ARG_ENABLE([rfcomm],
	[AS_HELP_STRING([--enable-rfcomm], [enable building of bluealsa-rfcomm tool])])
AM_CONDITIONAL([ENABLE_RFCOMM], [test "x$enable_rfcomm" = "xyes"])

AC_ARG_ENABLE([hcitop],
	[AS_HELP_STRING([--enable-hcitop], [enable building of hcitop tool])])
AM_CONDITIONAL([ENABLE_HCITOP], [test "x$enable_hcitop" = "xyes"])
AM_COND_IF([ENABLE_HCITOP], [
	PKG_CHECK_MODULES([LIBBSD], [libbsd])
	PKG_CHECK_MODULES([NCURSES], [ncurses])
])

AC_ARG_ENABLE([test],
	[AS_HELP_STRING([--enable-test], [enable unit test])])
AM_CONDITIONAL([ENABLE_TEST], [test "x$enable_test" = "xyes"])
AM_COND_IF([ENABLE_TEST], [
	PKG_CHECK_MODULES([CHECK], [check >= 0.9.10])
])

AC_ARG_WITH([dbusconfdir],
	AC_HELP_STRING([--with-dbusconfdir=dir], [path to D-Bus system bus configuration files]),
	[dbusconfdir="${withval}"],
	[dbusconfdir=$($PKG_CONFIG --variable=sysconfdir dbus-1)/dbus-1/system.d])
AC_ARG_WITH([alsaplugindir],
	AS_HELP_STRING([--with-alsaplugindir=dir], [path where ALSA plugin files are stored]),
	[alsaplugindir="$withval"],
	[alsaplugindir=$($PKG_CONFIG --variable=libdir alsa)/alsa-lib])
AC_ARG_WITH([alsaconfdir],
	AS_HELP_STRING([--with-alsaconfdir=dir], [path to ALSA add-on configuration files]),
	[alsaconfdir="$withval"],
	[AM_COND_IF([ALSA_1_1_7],
		[alsaconfdir="$sysconfdir/alsa/conf.d"],
		[alsaconfdir="$datadir/alsa/alsa.conf.d"])])

test "x$prefix" = xNONE && prefix=$ac_default_prefix
test "x$exec_prefix" = xNONE && exec_prefix=$prefix

# TODO: Get rid of "ev(a|i)l" statements.
# TIP: Wizard-level Autotconf coder is needed.
eval alsaconfdir="$alsaconfdir"
eval alsaconfdir="$alsaconfdir"
eval alsaplugindir="$alsaplugindir"
eval alsaplugindir="$alsaplugindir"

AC_DEFINE_UNQUOTED([ALSA_CONF_DIR], "$alsaconfdir", [Directory containing ALSA add-on configuration files.])
AC_DEFINE_UNQUOTED([ALSA_PLUGIN_DIR], "$alsaplugindir", [Directory containing ALSA add-on modules.])

AC_SUBST([DBUS_CONF_DIR], [$dbusconfdir])
AC_SUBST([ALSA_CONF_DIR], [$alsaconfdir])
AC_SUBST([ALSA_PLUGIN_DIR], [$alsaplugindir])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/asound/Makefile
	utils/Makefile
	test/Makefile])
AC_OUTPUT

# warn user that alsa-lib thread-safety makes troubles
AM_COND_IF([ALSA_1_1_2], [
	AC_MSG_WARN([              *** alsa-lib >= 1.1.2 ***])
	AC_MSG_WARN([Starting from alsa-lib 1.1.2, it is possible to enable])
	AC_MSG_WARN([thread-safe API functions. Unfortunately, this feature])
	AC_MSG_WARN([is not mature enough - software plugins may experience])
	AC_MSG_WARN([random deadlocks (bluez-alsa PCM plugin is affected).])
	AC_MSG_WARN([Either compile alsa-lib without this feature or export])
	AC_MSG_WARN([LIBASOUND_THREAD_SAFE=0 while using bluealsa PCM.])
])

# warn user that aptX support is not an open-source feature
AM_COND_IF([ENABLE_APTX], [
	AC_MSG_WARN([             *** aptX encoder support ***])
	AC_MSG_WARN([You have enabled support for aptX encoding. Note, that])
	AC_MSG_WARN([by default bluez-alsa will use openaptx library, which])
	AC_MSG_WARN([does NOT perform any encoding/decoding.])
])
