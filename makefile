#
# Makefile for Elite - The New Kind.
#
CC   ?= gcc
WRES ?= windres

# Location of the Allegro 5 SDK. The defaults match a standard MSYS2/
# MinGW-w64 installation on 64-bit Windows 10 (installed to C:\msys64).
# Override ALLEGRO_DIR if you installed the Allegro SDK elsewhere.
ALLEGRO_DIR ?= C:/msys64/mingw64
ALLEGRO_INC ?= "$(ALLEGRO_DIR)/include"
ALLEGRO_LIB ?= "$(ALLEGRO_DIR)/lib"

CFLAGS  += -O2 -Wall -I$(ALLEGRO_INC)
LDFLAGS += -L$(ALLEGRO_LIB)

# Link against the Allegro 5 monolith library plus the standard Win32 libs.
LIBS    = -mwindows \
          -lallegro_monolith \
          -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lole32 -lwinmm

OBJS    = alg_gfx.o alg_main.o docked.o elite.o \
          intro.o planet.o shipdata.o shipface.o sound.o space.o \
          swat.o threed.o vector.o random.o trade.o options.o \
          stars.o missions.o nkres.o pilot.o file.o keyboard.o

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

newkind.exe: $(OBJS)
	$(CC) $(LDFLAGS) -o newkind.exe $(OBJS) $(LIBS)

nkres.o: nkres.rc
        $(WRES) nkres.rc nkres.o

.PHONY: clean

clean:
	$(RM) $(OBJS) newkind.exe


alg_gfx.o: alg_gfx.c alg_data.h config.h elite.h planet.h gfx.h

alg_main.o: alg_main.c alg_data.h config.h elite.h planet.h gfx.h docked.h\
	intro.h shipdata.h shipface.h space.h main.h pilot.h file.h keyboard.h

docked.o: docked.c config.h elite.h planet.h gfx.h

elite.o: elite.c config.h elite.h planet.h vector.h shipdata.h

intro.o: intro.c space.h config.h elite.h planet.h gfx.h vector.h\
	shipdata.h shipface.h threed.h

planet.o: planet.c config.h elite.h planet.h

shipdata.o: shipdata.c shipdata.h vector.h

shipface.o: shipface.c config.h elite.h planet.h shipface.h gfx.h

threed.o: threed.c space.h config.h elite.h planet.h gfx.h vector.h shipdata.h\
	shipface.h threed.h

vector.o: vector.c config.h vector.h

sound.o: sound.c sound.h

space.o: space.c space.h vector.h alg_data.h config.h elite.h planet.h\
	gfx.h docked.h intro.h shipdata.h shipface.h main.h random.h

swat.o: swat.c swat.h elite.h config.h main.h gfx.h alg_data.h shipdata.h\
	random.h pilot.h

random.o: random.c random.h

trade.o: trade.c trade.h elite.h config.h

options.o: options.c options.h elite.h config.h gfx.h file.h

stars.o: stars.c stars.h elite.h config.h gfx.h random.h

missions.o: missions.c missions.h config.h elite.h gfx.h planet.h main.h\
	vector.h space.h

pilot.o: pilot.c pilot.h config.h elite.h gfx.h vector.h space.h main.h

file.o: file.c file.h config.h elite.h

keyboard.o: keyboard.c keyboard.h
