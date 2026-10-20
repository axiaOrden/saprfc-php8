dnl config.m4 for extension saprfc (PHP 8.3+)
dnl
dnl SAPRFC links against the classic SAP RFC SDK (saprfc.h, sapitab.h,
dnl rfcsi.h and librfccm). It does NOT support the NetWeaver RFC SDK 7.50+.
dnl
dnl Usage:
dnl   phpize-legacy
dnl   ./configure --with-php-config=$(command -v php-config-legacy) \
dnl               --with-saprfc=/opt/rfcsdk
dnl   make -j$(nproc)

PHP_ARG_WITH([saprfc], [for saprfc support],
[  --with-saprfc[=DIR]     Include saprfc support. DIR is the classic SAP
                          RFCSDK install directory (default search:
                          /usr/sap/rfcsdk /usr/local/rfcsdk /opt/rfcsdk).],
[no])

if test "$PHP_SAPRFC" != "no"; then
  dnl Resolve a relative DIR before it is embedded in linker flags.
  case "$PHP_SAPRFC" in
    /*) saprfc_search="$PHP_SAPRFC" ;;
    *)  saprfc_search="$(cd "$PHP_SAPRFC" 2>/dev/null && pwd)" ;;
  esac
  if test -z "$saprfc_search"; then
    AC_MSG_ERROR([SAP RFCSDK directory '$PHP_SAPRFC' does not exist])
  fi

  SAPRFC_DIR=
  for i in "$saprfc_search" /usr/sap/rfcsdk /usr/local/rfcsdk /opt/rfcsdk; do
    if test -f "$i/include/saprfc.h"; then
      SAPRFC_DIR="$i"
      break
    fi
  done

  if test -z "$SAPRFC_DIR"; then
    AC_MSG_ERROR([Cannot find the classic SAP RFCSDK (include/saprfc.h).
Use --with-saprfc=DIR pointing at an RFCSDK 6.20/6.40 installation.
The NetWeaver RFC SDK 7.50+ is not compatible with this extension.])
  fi

  AC_MSG_CHECKING([for classic SAP RFCSDK in $SAPRFC_DIR])
  AC_MSG_RESULT([yes])

  if grep -q SAPRFC_MOCK_SDK "$SAPRFC_DIR/include/saprfc.h" 2>/dev/null; then
    AC_MSG_WARN([*** Using the SAPRFC test-double RFCSDK, NOT the real SAP SDK. ***])
    AC_MSG_WARN([*** This build cannot connect to a real SAP system.             ***])
  fi

  SAPRFC_LIBDIR=
  SAPRFC_LIBNAME=
  for d in "$SAPRFC_DIR/lib" "$SAPRFC_DIR/lib64"; do
    if test -f "$d/librfccm.so"; then
      SAPRFC_LIBDIR="$d"
      SAPRFC_LIBNAME=rfccm
      break
    elif test -f "$d/librfc.so"; then
      SAPRFC_LIBDIR="$d"
      SAPRFC_LIBNAME=rfc
      break
    elif test -f "$d/librfc.a"; then
      SAPRFC_LIBDIR="$d"
      SAPRFC_LIBNAME=rfc
      break
    fi
  done

  if test -z "$SAPRFC_LIBDIR"; then
    AC_MSG_ERROR([Cannot find librfccm.so (or librfc) under $SAPRFC_DIR/lib.
Install the 64-bit classic SAP RFCSDK library.])
  fi

  PHP_ADD_INCLUDE([$SAPRFC_DIR/include])
  PHP_ADD_LIBRARY_WITH_PATH([$SAPRFC_LIBNAME], [$SAPRFC_LIBDIR], [SAPRFC_SHARED_LIBADD])
  dnl Make the built extension find the SDK without requiring LD_LIBRARY_PATH.
  SAPRFC_SHARED_LIBADD="$SAPRFC_SHARED_LIBADD -Wl,-rpath,$SAPRFC_LIBDIR"
  PHP_SUBST([SAPRFC_SHARED_LIBADD])

  AC_CANONICAL_HOST
  case "$host" in
    *-hp-*)
      PHP_ADD_LIBRARY_WITH_PATH([cl], [$SAPRFC_LIBDIR], [SAPRFC_SHARED_LIBADD])
      ;;
  esac

  PHP_NEW_EXTENSION([saprfc], [saprfc.c rfccal.c], [$ext_shared],,
                    [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1])
fi
