SRC = pong.c
BIN = pong

$(BIN): $(SRC)
	cc -o $@ $^ -lglfw -lGL -lpng -lm -lz
