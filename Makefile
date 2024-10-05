CC = gcc
CFLAGS = -Wall -Wextra -g -ggdb -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -pg
LDFLAGS = -lncurses

SRC = $(filter-out battletank_server.c, $(wildcard *.c))
OBJ = $(SRC:.c=.o)
EXEC = battletank-client

SERVER_SRC = battletank_server.c protocol.c network.c game_state.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)
SERVER_EXEC = battletank-server

all: $(EXEC) $(SERVER_EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean

