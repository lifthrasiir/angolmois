SRC = angolmois.c
BIN = angolmois
CFLAGS = -Os -Wunused -Wall -W -std=c99


.PHONY: all clean

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) `sdl-config --cflags --libs` -lSDL_mixer -lSDL_image -lsmpeg -o $(BIN)

clean:
	rm -rf $(BIN)

