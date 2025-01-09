#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/user/proj/trim/xbuild
  make -f /Users/user/proj/trim/xbuild/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/user/proj/trim/xbuild
  make -f /Users/user/proj/trim/xbuild/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/user/proj/trim/xbuild
  make -f /Users/user/proj/trim/xbuild/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/user/proj/trim/xbuild
  make -f /Users/user/proj/trim/xbuild/CMakeScripts/ReRunCMake.make
fi

