SRC = angolmois.c
BIN = angolmois
CFLAGS = -Os -Wunused -Wall


.PHONY: all clean

all: $(BIN)

ifeq ($(shell uname),Darwin)
$(BIN): $(SRC) SDLmain.m
	$(CC) $(CFLAGS) $(SRC) `sdl-config --cflags --libs` -lSDL_mixer -lSDL_image -lsmpeg SDLmain.m -o $(BIN)
else
$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) `sdl-config --cflags --libs` -lSDL_mixer -lSDL_image -lsmpeg -o $(BIN)
endif

clean:
	rm -rf $(BIN)

