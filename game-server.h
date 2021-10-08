#ifndef __GAME_SERVER_H__
#define __GAME_SERVER_H__

#include <memory>
#include <map>
#include <set>
#include <cstring>
#include <vector>

#include "receiver.h"
#include "generator.h"

#define buffer_size 548

bool players_cmp(std::shared_ptr<player> A, std::shared_ptr<player> B);
struct compare {
    bool operator()(std::shared_ptr<player> A, 
                    std::shared_ptr<player> B) const {
        return players_cmp(A,B); // Implementacja w game-server.cpp
    }
};

using clients_map = std::map<std::pair<in_port_t, std::string>,
                            std::shared_ptr<client_opt>>;
using players_set = std::multiset<std::shared_ptr<player>, compare>;
using board_map = std::map<std::pair<uint32_t, uint32_t>, bool>;
using events_vector = std::vector<event*>;
using names_set = std::set<std::string>;

class Server {

private:
    const uint8_t minimal_players_nr = 2;
    const uint8_t maximal_players_nr = 25;

    std::shared_ptr<std::array<char, buffer_size>> buffer;
    int turning_speed, rounds_per_sec, maxx, maxy, sock;
    clients_map clients;
    players_set players;
    names_set all_names;
    board_map board;
    events_vector events;
    uint32_t game_id;
    uint32_t be_game_id; // game_id w sieciowej kolejności bajtów.
    int clicked_sth_nr;
    int players_nr;
    std::shared_ptr<Generator> generator;
    bool during_game;

    void round_exec();
    void init_game();
    void clean_clients();
    void process_request(Receiver& r);
    void event_new_game();
    void event_pixel(uint32_t x, uint32_t y, uint8_t player_nr);
    void event_player_eliminated(std::shared_ptr<player>& el_player);
    void event_game_over();
    bool check_pixel(long double x, long double y);
    void clean_after_game();
    void send_to_client(std::shared_ptr<sockaddr_in6> address, uint32_t start_ev);
    void send_last_to_all();
public: 
    Server(int iturning_speed, int irounds_per_sec,
                         int iwidth, int iheight, int isock, uint32_t seed);
    void start();
};


#endif /* __GAME_SERVER_H__ */