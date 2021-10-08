#include <unistd.h>
#include <endian.h>
#include <string>

#include "game-client.h"
#include "game-util.h"
#include "client-receiver.h"


const char NEW_GAME[] = "NEW_GAME ";
const char PIXEL[] = "PIXEL ";
const char PLAYER_ELIMINATED[] = "PLAYER_ELIMINATED ";

const int cl_gs_len = 34;
// Bufor do wysyłania komunikatów do serwera gry.
char cl_gs_buf[cl_gs_len];
 
const int interface_buf_size = 600;
// Bufor do wysyłania komunikatów do interfejsu.
char cl_interface_buf[interface_buf_size]; 


Client::Client(int g_sock, int i_sock, char* name, int name_len) {
    game_sock = g_sock;
    interface_sock = i_sock;

    left_clicked = false;
    right_clicked = false;
    during_game = false;
    game_id = 0;

    set_id(now());
    set_next_expected(0);
    set_turn_direction(0);
    // Zapisujemy nazwę gracza w buforze komunikatów wysyłanych do serwera.
    memcpy(cl_gs_buf+13, name, name_len);

    cl_gs_size = 13 + name_len;
}

void Client::start() {
    // Komunikaty interfejsu.
    int com; 

    // Czas rozpoczęcia wysłania poprzedniego komunikatu do serwera gry.
    uint64_t prev_com = now();
    
    for (;;) {
        deadline = prev_com + 30000;

        do {
            // Sprawdzamy komunikaty od interfejsu.
            if ((com = interface_next(interface_sock)) != 0) {
                set_proper_td(com);
            }
            // Sprawdzamy wiadomości od serwera gry.
            read_game_server_com();
        } while(deadline >= now());

        prev_com = now();

        send_to_game_server();
    }

}

// Ustawia odpowiednią wartość turn_direction w zależności
// od ostatniego komunikatu interfejsu graficznego i poprzedniej
// wartości. Zakładamy, że wartość com jest spośród: -2, -1, 1, 2
// tak jak w funkcji interface_next().
void Client::set_proper_td(int com) {
    if (com > 0) {
        if (com == 1) {
            right_clicked = true;
        }
        else {
            left_clicked = true;
        }
        set_turn_direction(com);
    }
    // Odkliknięto prawy ale lewy był wcześniej wciśnięty.
    else if (com == -1 && left_clicked) {
        right_clicked = false;
        set_turn_direction(2);
    }
    // Odkliknięto lewy ale prawy był wcześniej wciśnięty.
    else if (com == -2 && right_clicked) {
        left_clicked = false;
        set_turn_direction(1);
    }
    else {
        left_clicked = false;
        right_clicked = false;
        set_turn_direction(0);
    }
}

// Wysyła komunikat do serwera gry.
void Client::send_to_game_server() {
    sent_code = write(game_sock, cl_gs_buf, cl_gs_size);
}

// Ustawia id klienta wysyłane do serwera gry.
void Client::set_id(uint64_t id) {
    id = htobe64(id);
    memcpy(cl_gs_buf, &id, 8);
}

// Ustawia turn_direction wysyłane do serwera gry.
void Client::set_turn_direction(uint8_t turn_direction) {
    current_turn_direction = turn_direction;
    memcpy(cl_gs_buf+8, &turn_direction, 1);
}

// Ustawia next_expected_event_no wysyłane do serwera gry
void Client::set_next_expected(uint32_t next) {
    next_expected = next;
    next = htobe32(next);
    memcpy(cl_gs_buf+9, &next, 4);
}

// Próbuje odebrać i przetworzyć komunikat od serwera gry.
void Client::read_game_server_com() {
    bool err = false;
    uint32_t new_game_id = get_uint32(err, game_sock);
    // Jeśli trwa partia to ignorujemy komunikatu z innym niż jej id.
    if (err || (during_game && new_game_id != game_id)) {
        reset_game_buffer();
        return;
    }
    while (!err || deadline >= now()) {
        // Zapisujemy odpowiednie bajty by sprawdzić potem sumę kontrolną.
        start_checking_bytes();

        uint32_t len = get_uint32(err, game_sock);
        if (err) {
            return;
        }
        // Być może wczytaliśmy już kolejny komunikat.
        if (len == game_id) {
            start_checking_bytes();
            len = get_uint32(err, game_sock);
            if (err) {
                return;
            }
        }

        uint32_t event_no = get_uint32(err, game_sock);
        if (err) {
            return;
        }
        uint8_t event_type = get_uint8(err, game_sock);
        if (err) {
            return;
        }

        switch (event_type) {
            case 0:
                err = event_new_game(len, new_game_id);
                break;
            case 1:
                err = event_pixel();
                break;
            case 2:
                err = event_player_eliminated();
                break;
            case 3:
                err = event_game_over();
                break;
            default:
                break;
        }
        if (err) {
            return;
        }

        int32_t crc32_value = get_uint32(err, game_sock);
        if (err) {
            return;
        }
        // Sprawdzamy czy suma kontrolna poprawna.
        if (!check_crc32(crc32_value)) {
            return;
        }
        // Jeśli brak błędów i jest coś to wysłania do interfejsu to to robimy.
        if (destination > cl_interface_buf) {
            sent_code = write(interface_sock, cl_interface_buf,
                                     destination-cl_interface_buf);  
        }
        if (event_no >= next_expected) {
            set_next_expected(event_no+1);
        }
    }
}

// Zwraca true w przypadku błędu.
bool Client::event_new_game(int len, uint32_t new_game_id) {
    bool err = false;
    uint32_t maxx = get_uint32(err, game_sock);
    if (err) {
        return true;
    }
    uint32_t maxy = get_uint32(err, game_sock);
    if (err) {
        return true;
    }
    // Czyścimy po poprzednich grach.
    players.clear(); 
    // Oczekiwana liczba bajtów dotyczących nazw graczy.
    len -= 13; 
    while (len > 0) {
        auto str_ptr = get_string(err, game_sock);
        if (err) {
            return true;
        }
        players.push_back(str_ptr);
        len -= str_ptr->size()+1;
    }

    send_new_game(maxx, maxy);
    game_id = new_game_id;
    during_game = true;
    return false;
}

// Zwraca true w przypadku błędu.
bool Client::event_pixel() {
    bool err = false;
    uint8_t player = get_uint8(err, game_sock);
    if (err) {
        return true;
    }
    uint32_t x = get_uint32(err, game_sock);
    if (err) {
        return true;
    }
    uint32_t y = get_uint32(err, game_sock);
    if (err) {
        return true;
    }

    send_pixel(player, x, y);
    return false;
}

// Zwraca true w przypadku błędu.
bool Client::event_player_eliminated() {
    bool err = false;
    uint8_t player = get_uint8(err, game_sock);
    if (err) {
        return true;
    }

    send_player_eliminated(player);
    return false;
}

// Zwraca true w przypadku błędu.
bool Client::event_game_over() {
    during_game = false;
    return false;
}

// Przygotowuje wysyłanie do interfejsu zdarzenia NEW GAME.
void Client::send_new_game(uint32_t maxx, uint32_t maxy) {
    destination = cl_interface_buf;
    memcpy(destination, &NEW_GAME, sizeof(NEW_GAME)-1);
    destination += sizeof(NEW_GAME)-1;

    auto maxx_str = std::to_string(maxx);
    auto maxy_str = std::to_string(maxy);

    memcpy(destination, maxx_str.c_str(), maxx_str.size());
    destination += maxx_str.size();
    destination[0] = ' ';
    destination += 1;
    memcpy(destination, maxy_str.c_str(), maxy_str.size());
    destination += maxy_str.size();
    destination[0] = ' ';
    destination += 1;

    for (size_t i = 0; i < players.size(); i++) {
        memcpy(destination, players[i]->c_str(), players[i]->size());
        destination += players[i]->size();
        if (i+1 != players.size()) {
            destination[0] = ' ';
            destination += 1;
        }
    }

    destination[0] = '\n';
    destination++;
}

// Przygotowuje wysyłanie do interfejsu zdarzenia PIXEL.
void Client::send_pixel(uint8_t player, uint32_t x, uint32_t y) {
    destination = cl_interface_buf;
    memcpy(destination, &PIXEL, sizeof(PIXEL)-1);
    destination += sizeof(PIXEL)-1;

    auto x_str = std::to_string(x);
    auto y_str = std::to_string(y);


    memcpy(destination, x_str.c_str(), x_str.size());
    destination += x_str.size();
    destination[0] = ' ';
    destination += 1;
    memcpy(destination, y_str.c_str(), y_str.size());
    destination += y_str.size();
    destination[0] = ' ';
    destination++;

    memcpy(destination, players[player]->c_str(), players[player]->size());
    destination += players[player]->size();

    destination[0] = '\n';
    destination++;
}

// Przygotowuje Wysyłanie do interfejsu zdarzenia PLAYER ELIMINATED.
void Client::send_player_eliminated(uint8_t player) {
    destination = cl_interface_buf;
    memcpy(destination, &PLAYER_ELIMINATED, sizeof(PLAYER_ELIMINATED)-1);
    destination += sizeof(PLAYER_ELIMINATED)-1;

    memcpy(destination, players[player]->c_str(), players[player]->size());
    destination += players[player]->size();

    destination[0] = '\n';
    destination++;
}