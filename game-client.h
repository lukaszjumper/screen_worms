#ifndef __GAME_CLIENT_H__
#define __GAME_CLIENT_H__

#include <vector>
#include <memory>
#include "types.h"

class Client {
private:
    int game_sock;
    int interface_sock;
    uint8_t current_turn_direction;
    uint32_t next_expected;
    bool left_clicked;
    bool right_clicked;
    char *name;
    int cl_gs_size; // Wielkość komunikatu do serwera gry.
    bool during_game;
    uint32_t game_id;
    char *destination; // Wskaźnik końca tablicy bajtów do wysłania.
    int sent_code;
    uint64_t deadline;

    std::vector<std::shared_ptr<std::string>> players; // Nazwy graczy.

    void send_to_game_server();
    void set_id(uint64_t id);
    void set_next_expected(uint32_t next);
    void set_turn_direction(uint8_t turn_direction);
    void set_proper_td(int com);
    void read_game_server_com();
    bool event_new_game(int len, uint32_t new_game_id);
    bool event_pixel();
    bool event_player_eliminated();
    bool event_game_over();
    void send_new_game(uint32_t maxx, uint32_t maxy);
    void send_pixel(uint8_t player, uint32_t x, uint32_t y);
    void send_player_eliminated(uint8_t player);
public:
    Client(int g_sock, int i_sock, char *name, int name_len);
    void start();
    
};


#endif /* __GAME_CLIENT_H__ */