SRC = angolmois.c
BIN = angolmois
CFLAGS = -Os


.PHONY: all clean

all: $(BIN)

ifeq ($(shell uname),Darwin)
$(BIN): $(SRC) SDLmain.m
	$(CC) $(CFLAGS) -framework Cocoa \
		-I/Library/Frameworks/SDL.framework/Headers -framework SDL \
		-I/Library/Frameworks/SDL_image.framework/Headers -framework SDL_image \
		-I/Library/Frameworks/SDL_mixer.framework/Headers -framework SDL_mixer \
		$(SRC) SDLmain.m -o $(BIN)
else
$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lSDL -lSDL_mixer -lSDL_image -I/usr/include/SDL -o $(BIN)
endif

clean:
	rm -rf $(BIN)

