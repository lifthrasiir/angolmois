SRC = angolmois.c
BIN = angolmois
CFLAGS = -Os -Wunused -Wall -W -std=c99


.PHONY: all clean

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) `pkg-config --cflags --libs sdl SDL_image SDL_mixer` `smpeg-config --cflags --libs` -o $(BIN)

clean:
	rm -rf $(BIN)

