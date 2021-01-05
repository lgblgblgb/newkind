ARCH		= native

DEPFILE		= newkind-$(ARCH).depend

CC_native	= gcc
CC_win64	= x86_64-w64-mingw32-gcc
CC_win32	= i686-w64-mingw32-gcc
CC		= $(CC_$(ARCH))

SRCS		= datafilebank.c SDL2_gfxPrimitives.c SDL2_rotozoom.c sdl.c main.c docked.c elite.c intro.c planet.c shipdata.c shipface.c sound.c space.c swat.c threed.c vector.c random.c trade.c options.c stars.c missions.c pilot.c file.c keyboard.c
OBJS		= $(SRCS:.c=.o)

SDL_CFG_native	= sdl2-config
SDL_CFG_win64	= x86_64-w64-mingw32-sdl2-config
SDL_CFG_win32	= i686-w64-mingw32-sdl2-config

DLL		= SDL2.dll
DLL_SOURCE	= $(shell $(SDL_CFG_$(ARCH)) --prefix)/bin/$(DLL)

CFLAGS_native	= -std=c99 -pipe -Ofast -ffast-math -fno-common -falign-functions=16 -falign-loops=16 -Wall -I. $(shell $(SDL_CFG_native) --cflags)
#CFLAGS_native	= -g -std=c99 -pipe -O0 -fno-common -falign-functions=16 -falign-loops=16 -Wall -g -I. $(shell $(SDL_CFG_native) --cflags)
CFLAGS_win64	= -std=c99 -pipe -Ofast -ffast-math -fno-common -falign-functions=16 -falign-loops=16 -Wall -I. $(shell $(SDL_CFG_win64) --cflags)
CFLAGS_win32	= -std=c99 -pipe -Ofast -ffast-math -fno-common -falign-functions=16 -falign-loops=16 -Wall -I. $(shell $(SDL_CFG_win32) --cflags)
CFLAGS		= $(CFLAGS_$(ARCH))

LDFLAGS_native	= $(shell $(SDL_CFG_native) --libs) -lm
LDFLAGS_win64	= $(shell $(SDL_CFG_win64) --libs)
LDFLAGS_win32	= $(shell $(SDL_CFG_win32) --libs)
LDFLAGS		= $(LDFLAGS_$(ARCH))

EXE_native	= newkind
EXE_win64	= newkind-win64.exe
EXE_win32	= newkind-win32.exe
EXE		= $(EXE_$(ARCH))

ALLDEPS		= Makefile

all:
	$(MAKE) dep ARCH=$(ARCH)
	$(MAKE) $(EXE) ARCH=$(ARCH)

datafilebank.c: data/*
	data/datafile.sh > $@

.c.o: $(ALLDEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(DLL):
	test -s $(DLL_SOURCE) && cp $(DLL_SOURCE) $(DLL) || true

$(EXE): $(OBJS) $(ALLDEPS)
	$(MAKE) $(DLL) ARCH=$(ARCH)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

dep:	datafilebank.c
	$(MAKE) $(DEPFILE) ARCH=$(ARCH)

$(DEPFILE): $(ALLDEPS) *.c *.h datafilebank.c
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPFILE)

clean:
	rm -f $(OBJS) $(EXE) $(DLL)

distclean:
	$(MAKE) clean ARCH=$(ARCH)
	rm -f $(DEPFILE) *~ *.swp a.out datafilebank.c

.PHONY: dep all clean distclean

ifneq ($(wildcard $(DEPFILE)),)
include $(DEPFILE)
endif