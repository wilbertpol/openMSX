# $Id$
#
# Configuration for NetBSD.

# Does platform support symlinks?
USE_SYMLINK:=true

# File name extension of executables.
EXEEXT:=

COMPILE_FLAGS+=-D_REENTRANT -D_THREAD_SAFE
LINK_FLAGS+=-pthread
