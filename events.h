#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "game-server.h"

event *create_e_new_game(uint32_t event_no, uint32_t maxx, uint32_t maxy,
                                                         players_set& players);
event *create_e_pixel(uint32_t event_no, uint8_t player_no,
                                             uint32_t x, uint32_t y);
event *create_e_player_eliminated(uint32_t event_no, uint8_t player_no);
event *create_e_game_over(uint32_t event_no);
void destroy_event(event *event_to_destroy);


#endif /* __EVENTS_H__ */