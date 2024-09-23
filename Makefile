CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses

SRC = $(filter-out battletank-server.c, $(wildcard *.c))
OBJ = $(SRC:.c=.o)
EXEC = battletank-client

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean

