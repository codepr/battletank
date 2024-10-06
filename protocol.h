#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>

#include "game_state.h"

void bin_write_i32(unsigned char *buf, unsigned long val);
long int bin_read_i32(const unsigned char *buf);
int protocol_serialize_action(unsigned action, unsigned char *buf);
int protocol_deserialize_action(const unsigned char *buf, unsigned *action);
int protocol_serialize_game_state(const Game_State *state, unsigned char *buf);
int protocol_deserialize_game_state(const unsigned char *buf,
                                    Game_State *state);

#endif
