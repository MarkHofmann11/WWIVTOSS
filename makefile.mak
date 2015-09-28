#############################################################################
# WWIVToss Makefile for TCC
#

.autodepend

#############################################################################
# Define the paths for the various types of files.

SRC=.
OBJ=.\obj
EXE=.\exe
REL=.\release

.path.c=$(SRC)
.path.cpp=$(SRC)
.path.h=$(SRC)
.path.obj=$(OBJ)
.path.exe=$(EXE)

#############################################################################
# Define command lines to use.

LIB87=emu
COPTS= -G -O -vi- -Z -ml -c -k- -wuse -wpro -weas -wpre -y -v
LIBPATH=C:\tc\lib
LIBFILES=$(LIB87)+mathl+cl+..\lib\spawnl
CFLAGS = $(COPTS) -Y -n$(OBJ) { $< }
LDFLAGS=/c /x /v
LD=tlink
LINK=$(LD) $(LDFLAGS)

#############################################################################
# generic compilation rule
.cpp.obj:
  $(CC) $(CFLAGS)
.c.obj:
  $(CC) $(CFLAGS)

#############################################################################
# Build major component inclusions
all: wwivtoss.exe

#############################################################################
# Build individual component rules
wwivtoss.exe: wwivtoss.obj share.obj export.obj dawg.obj
  $(LINK) /L$(LIBPATH) @&&|
c0l $**
$<,,$(LIBFILES)
|

clean:
	-del $(OBJ)\*.*
	-del $(EXE)\*.*

