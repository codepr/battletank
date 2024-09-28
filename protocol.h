#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>

#include "game_state.h"

void bin_write_i32(char *buf, int val);
int bin_read_i32(const char *buf);
int protocol_serialize_action(unsigned action, char *buf);
int protocol_deserialize_action(const char *buf, unsigned *action);
int protocol_serialize_game_state(const Game_State *state, char *buf);
int protocol_deserialize_game_state(const char *buf, Game_State *state);

#endif
