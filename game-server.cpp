#include <cstring>
#include <string>
#include <cmath>
#include <tuple>
#include <array>
#include <endian.h>

#include "game-server.h"
#include "events.h"
#include "game-util.h"


Server::Server(int iturning_speed, int irounds_per_sec, 
                            int iwidth, int iheight, int isock, uint32_t seed) {
    turning_speed = iturning_speed;
    rounds_per_sec = irounds_per_sec;
    maxx = iwidth;
    maxy = iheight;
    sock = isock;
    clicked_sth_nr = 0;
    players_nr = 0;
    generator = std::make_shared<Generator>(seed);
    during_game = false; // Na początku nie ma żadnej partii.
    buffer = std::make_shared<std::array<char, buffer_size>>();
}

void Server::start() {
    uint64_t deadline;
    // Czas rozpoczęcia obliczania rezultatów poprzedniej tury.
    uint64_t prev_round = now();
    Receiver receiver(sock);

    for (;;) {
        deadline = prev_round + 1000000/rounds_per_sec;
        // Odbieramy komunikaty aż upłynie wyznaczony czas.
        do {
            if (receiver.next_receive()) {
                process_request(receiver);
            }
        } while(deadline > now());
    
        // Zaczynamy obliczać rezultaty kolejnej tury.
        prev_round = now();

        clean_clients();

        if (during_game) {
            round_exec();
        }

        if (!during_game && players_nr >= minimal_players_nr 
                                    && players_nr == clicked_sth_nr) {
            init_game();
        }
    }
}

// Przetwarza komunikat od klienta.
void Server::process_request(Receiver& r) {
    // Sprawdzamy czy taki klient o danym ip oraz porcie już jest w bazie.
    auto sock_id = std::make_pair(r.get_port_nr(), *r.get_ip());
    auto client_it = clients.find(sock_id);
    std::shared_ptr<client_opt> client;
    if (client_it == clients.end()) {
        auto new_name = r.get_name();
        // Sprawdzamy czy takiej nazwy już nie było.
        if (new_name->compare("") != 0 
                && all_names.find(*new_name) != all_names.end()) {
            return; // Ignorujemy.
        }
        all_names.insert(*new_name);
        // Tworzymy nową strukturę.
        client = std::make_shared<client_opt>();
        client->this_player = std::make_shared<player>();
        client->address = r.get_address();
        client->id = r.get_id();
        client->clicked_sth = false;
        client->this_player->name = new_name;
        client->accepted = false; //Czy pozwalamy na udział rozstrzyga się później.
        clients.insert({sock_id, client});
    }
    else {
        // Jeśli taki klient już jest.
        client = client_it->second;
    }

    // Ignorujemy żadania o mniejszym id.
    if (client->id > r.get_id()) {
        return;
    }
    client->id = r.get_id();

    client->last_request = now(); // Czas ostatniego żądania.
    client->this_player->direction_change = r.get_direction();

    // Czy pozwolić na czynny udział w grze.
    if (!client->accepted && players_nr < maximal_players_nr 
                            && client->this_player->name->compare("")) {
        client->accepted = true;
        players_nr++;
    }
    // Jeśli gracz który miał zagrać przestał przysyłać nazwę to go wyrzucamy.
    if (client->accepted && !client->this_player->name->compare("")) {
        client->accepted = false;
        players_nr--;
        if (client->clicked_sth) {
            clicked_sth_nr--;
        }
    }

    // Informacja czy klient wcisnął strzałkę jest istotna dla rozpoczęcia partii.
    // Rozważamy jednak tylko czynnych graczy.
    if (client->accepted  && r.get_direction() != 0 && !client->clicked_sth) {
        client->clicked_sth = true;
        clicked_sth_nr++;
    }

    // Wysyłamy odpowiedź.
    send_to_client(client->address, r.get_next_expected_event_no());
}

// Przeprowadza kolejną turę.
void Server::round_exec() {
    for (auto p = players.begin(); p != players.end(); p++) {
        auto active_player = *p;
        if (active_player->is_playing) {
            if (active_player->direction_change == 1) {
                active_player->direction += turning_speed;
            }
            else if (active_player->direction_change == 2) {
                active_player->direction -= turning_speed;
            }

            // Zapisujemy na jakim pikselu jest robak.
            uint32_t prev_x = (uint32_t) floor(active_player->x);
            uint32_t prev_y = (uint32_t) floor(active_player->y);
            // Przesuwamy robaka.
            active_player->x += cos(M_PI * active_player->direction / 180);
            active_player->y += sin(M_PI * active_player->direction / 180);
            uint32_t new_x = (uint32_t) floor(active_player->x);
            uint32_t new_y = (uint32_t) floor(active_player->y);
            // Jeśli robak nie zmienił pozycji to nic się nie dzieje.
            if (prev_x == new_x && prev_y == new_y) {
                continue;
            }
           
            if (!check_pixel(active_player->x, active_player->y)) {
                event_player_eliminated(active_player);
                if (players_nr <= 1) {
                    event_game_over();
                    return;
                }
            }
            else {
                event_pixel(new_x, new_y, active_player->number);
            }
        }
    }
}

// Inicjuje kolejną partię.
void Server::init_game() {
    game_id = generator->get_random();
    be_game_id = htobe32(game_id);
    // Kompetujemy listę graczy.
    for(auto c = clients.begin(); c != clients.end(); c++) {
        // Tylko gracze zaakceptowani przez serwer.
        if (c->second->accepted) {
            players.insert(c->second->this_player);
            c->second->this_player->is_playing = true;
        }
    }

    during_game = true;
    event_new_game();

    for (auto [p, i] = std::tuple{players.begin(), 0}; 
                                            p != players.end(); p++, i++) {
        auto this_player = *p;
        this_player->x = (generator->get_random() % maxx) + 0.5;
        this_player->y = (generator->get_random() % maxy) + 0.5;
        this_player->direction = generator->get_random() % 360;
        this_player->number = i;

        if (!check_pixel(this_player->x, this_player->y)) {
            event_player_eliminated(this_player);
            if (players_nr <= 1) {
                event_game_over();
                return;
            }
        }
        else {
            event_pixel((uint32_t) this_player->x, (uint32_t) this_player->y, i);
        }
    }
}

// Jeśli nie było komunikatów od klienta przez 2 sekundy to funkcja ta go usuwa.
void Server::clean_clients() {
    for (auto c = clients.begin(); c != clients.end();) {
        if (c->second->last_request + 2000000 <= now()) {
            if (c->second->clicked_sth) {
                clicked_sth_nr--; 
            }
            if (c->second->accepted) {
                players_nr--;
            }
            all_names.erase(*(c->second->this_player->name));
            clients.erase(c++);
        }
        else {
            ++c;
        }
    }
}

// Zwraca true wtedy i tylko wtedy gdy pixel odpowiadający podanej pozycji jest
// na planszy i nie jest jedzony lub zjedzony.
bool Server::check_pixel(long double x, long double y) {
    if (x >= maxx || y >= maxy || x < 0 || y < 0) {
        return false;
    }
    if (board.find(std::make_pair((uint32_t) floor(x),(uint32_t) floor(y)))
                                                                != board.end()) {
        return false;
    }
    else {
        return true;
    }
}

// Generuje zdarzenie NEW GAME.
void  Server::event_new_game() {
    events.push_back(create_e_new_game(events.size(), maxx, maxy, players));
    send_last_to_all();
}

// Zaznacza pixel o danych współrzędnych jest jedzony/zjedzony
// i generuje zdarzenie pixel.
void Server::event_pixel(uint32_t x, uint32_t y, uint8_t player_nr) {
    board.insert({std::make_pair(x, y), true});
    events.push_back(create_e_pixel(events.size(), player_nr, x, y));
    send_last_to_all();
}

// Generuje zdarzenie PLAYER ELIMINATED
void Server::event_player_eliminated(std::shared_ptr<player>& el_player) {
    el_player->is_playing = false;
    players_nr--;
    events.push_back(create_e_player_eliminated(events.size(), el_player->number));
    send_last_to_all();
}

// Generuje zdarzenie GAME OVER.
void Server::event_game_over() {
    events.push_back(create_e_game_over(events.size()));
    send_last_to_all();

    clean_after_game();
}

// Usuwa dane z odpowiednich struktur i zwalnia pamięć.
// Powinna być wywołana po zakończeniu partii.
void Server::clean_after_game() {
    during_game = false;
    clicked_sth_nr = 0;
    players_nr = 0;
    // Przy następnej partii od nowa należy wybrać klientów do czynnego udziału.
    for (auto c = clients.begin(); c != clients.end(); c++) {
        c->second->accepted = false;
        c->second->clicked_sth = false;
    }
    players.clear();
    for (size_t i = 0; i < events.size(); i++) {
        destroy_event(events[i]);
    }
    events.clear();
    board.clear();
}

// Wysyła do danego klienta komunikat z wszystkimi zdarzeniami 
// o numerze co najmniej start_ev.
void Server::send_to_client(std::shared_ptr<sockaddr_in6> address, 
                                                        uint32_t start_ev) {
    if (start_ev >= events.size()) {
        return; // Nie ma nic do wysłania.
    }

    const socklen_t adr_len = sizeof(*address);                                              
    unsigned int bytes_count = 0; // Ile bajtów buffer już zapisano.

    for (auto i = start_ev; i < events.size(); i++) {
        if (bytes_count + events[i]->size > buffer_size) {
            sendto(sock, buffer.get()->data(), bytes_count, 0, 
                                  (sockaddr *) address.get(), adr_len);
            bytes_count = 0;
        }
        if (bytes_count == 0) {
            memcpy(buffer.get()->data(), &be_game_id, 4);
            bytes_count += 4;
        }
        memcpy(buffer.get()->data() + bytes_count,
                                    events[i]->content, events[i]->size);
        bytes_count += events[i]->size;
    }

    if (bytes_count > 0) {
        sendto(sock, buffer.get()->data(), bytes_count, 0,
                                  (sockaddr *) address.get(), adr_len);
    }
}

// Wysyła do wszystkich klientów komunikat o ostatnim zdarzeniu.
void Server::send_last_to_all() {
    for (auto c = clients.begin(); c != clients.end(); c++) {
        send_to_client(c->second->address, events.size()-1);
    }
}

bool players_cmp(std::shared_ptr<player> A, std::shared_ptr<player> B) {
    return A->name->compare(*B->name) < 0 ? true: false;
}