#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/phoenixjmh/proj/trim/xbin
  make -f /Users/phoenixjmh/proj/trim/xbin/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/phoenixjmh/proj/trim/xbin
  make -f /Users/phoenixjmh/proj/trim/xbin/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/phoenixjmh/proj/trim/xbin
  make -f /Users/phoenixjmh/proj/trim/xbin/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/phoenixjmh/proj/trim/xbin
  make -f /Users/phoenixjmh/proj/trim/xbin/CMakeScripts/ReRunCMake.make
fi

