.AUTODEPEND

.PATH.obj = Z:\BORLANDC\WWIVTOSS

#		*Translator Definitions*
CC = bcc +WWIVTOSS.CFG
TASM = tasm
TLIB = tlib
TLINK = tlink
LIBPATH = Z:\BORLANDC\LIB
INCLUDEPATH = Z:\BORLANDC\INCLUDE


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
 wwivtoss.obj \
 share.obj \
 export.obj \
 ..\..\borlandc\lib\spawnl.lib \
 dawg.obj

#		*Explicit Rules*
turbo assembler: wwivtoss.cfg $(EXE_dependencies)
  $(TLINK) /v/x/c/d/P-/Vt/L$(LIBPATH) @&&|
c0l.obj+
/v- z:\borlandc\wwivtoss\wwivtoss.obj+
z:\borlandc\wwivtoss\share.obj+
z:\borlandc\wwivtoss\export.obj+
z:\borlandc\wwivtoss\dawg.obj
z:\borlandc\wwivtoss\wwivtoss
		# no map file
/v+ ..\..\borlandc\lib\spawnl.lib+
emu.lib+
mathl.lib+
cl.lib
|


#		*Individual File Dependencies*
wwivtoss.obj: wwivtoss.cfg wwivtoss.c 

share.obj: wwivtoss.cfg share.c 

export.obj: wwivtoss.cfg export.c 

dawg.obj: wwivtoss.cfg dawg.c 

#		*Compiler Configuration File*
wwivtoss.cfg: wwivtoss.mak
  copy &&|
-ml
-G
-O
-Og
-Oe
-Om
-Ov
-Ol
-Ob
-Op
-Oi
-Z
-k-
-d
-vi-
-H=WWIVTOSS.SYM
-wpro
-wuse
-weas
-wpre
-nZ:\BORLANDC\WWIVTOSS
-I$(INCLUDEPATH)
-L$(LIBPATH)
-Ff
| wwivtoss.cfg


