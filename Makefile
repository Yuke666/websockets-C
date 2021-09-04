#windows

# CC=gcc

# CCFLAGS=-c -Wall -lm
# # SDL_LDFLAGS = $(shell sdl2-config --cflags --libs)
# # LDFLAGS=-Wall -L. -lYUGL -lmingw32 $(SDL_LDFLAGS) -lglew32 -lglu32 -lopengl32 -static-libgcc -lfmodex
# # LDFLAGS=-Wall -L. -lYUGL -lmingw32 -lSDL2 -lSDL2Main -lglew32 -lglu32 -lopengl32 -static-libgcc -lfmodex

# SOURCES=rectangle.c icon.res gamefuncs.c levels.c sprite.c camera.c levels/levelOne.c \
# levels/levelTwo.c levels/levelThree.c character.c levels/menu.c main.c

# OBJECTS=$(SOURCES:.cpp=.o)
# EXECUTABLE=hello.exe

# all: icon $(SOURCES) $(EXECUTABLE) clean
	
# $(EXECUTABLE): $(OBJECTS) 
# 	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# .cpp.o:
# 	$(CC) $(CFLAGS) $< -o $@

# icon: icon.rc 
# 	windres icon.rc -O coff -o icon.res 

# clean:
# 	rm -rf *.o *.res 


#linux

CC=gcc

CCFLAGS=-c -Wall -lm
LDFLAGS= -static-libgcc -lm   $(shell mysql_config --cflags --libs) -lpthread

SOURCES=main.c gameHandler.c cards.c websocket.c crypt.c

OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=server

all: $(SOURCES) $(EXECUTABLE) clean
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o *.res 

