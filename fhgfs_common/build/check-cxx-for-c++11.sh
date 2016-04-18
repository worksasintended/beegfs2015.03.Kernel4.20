#!/bin/sh

# this script will check if the given compiler (argument 1) is g++ and if it
# is at least version 4.8. if so, it will print "1" to stdout.
#
# this is intended to decide in Makefiles whether we automatically enable 
# building with c++11 or not.

# note: we only accept directly mathing name (and not substring match), because
# e.g. "clang++" also matches substring "g++" but has a different versioning
# scheme.
GXX_NAME=g++
GXX_MIN_CXX11_VER=4.8

# print help if no arguments given
if [ $# -eq 0 ] ; then
   echo "description: this script prints '1' if the given compiler is" \
      "$GXX_NAME with version $GXX_MIN_CXX11_VER or higher."
   echo
   echo "usage: $0 <compiler>"

   exit 1
fi

if [ "$1" = "$GXX_NAME" ]; then \
   GXX_VER=`$GXX_NAME -dumpversion | cut -d. -f1,2`; \
   echo $GXX_VER $GXX_MIN_CXX11_VER | awk '{if ($1 < $2) print 0; else print 1}'; \
fi
