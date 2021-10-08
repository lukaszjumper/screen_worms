#include <cstring>
#include <string>
#include <unistd.h>
#include <endian.h>
#include <memory>

#include "game-util.h"


const int interface_buf_size = 500;
const int interface_command_max_size = 20;

char interface_buf[interface_buf_size];
char interface_command[interface_command_max_size];

// Ile pól interface_buf zapisano.
int interface_max_index = 0;
// Następny bajt do przeczytania z interface_buf.
int inter_buf_index = 0;
// Gdzie zapisać kolejny bajt w interface_command.
int command_index = 0; 

// Odbiera komunikat od interfejsu użytkownika. Jako zwraca wartości:
// 0 = brak komunikatu, 2 = LEFT_KEY_DOWN, -2 = LEFT_KEY_UP,
// 1 = RIGHT_KEY_DOWN, -1 = RIGHT_KEY_UP
int interface_next(int sock) {
    command_index = 0;
    while (command_index < interface_command_max_size) {
        if (inter_buf_index >= interface_max_index) {
            // Wszystko jest przeczytane więc odbieramy kolejny komunikat.
            interface_max_index = read(sock, interface_buf, interface_buf_size);
            if (interface_max_index <= 0) {
                return 0;
            }
            inter_buf_index = 0;
        }
        // Komenda kończy się \n.
        if (interface_buf[inter_buf_index] == '\n') { 
            inter_buf_index++;
            interface_command[command_index] = 0;
            if (strcmp(interface_command, "LEFT_KEY_DOWN") == 0)
                return 2;
            if (strcmp(interface_command, "LEFT_KEY_UP") == 0)
                return -2;
            if (strcmp(interface_command, "RIGHT_KEY_DOWN") == 0)
                return 1;
            if (strcmp(interface_command, "RIGHT_KEY_UP") == 0)
                return -1;
            return 0;
        }
        
        interface_command[command_index] = interface_buf[inter_buf_index];
        command_index++;
        inter_buf_index++;
    }

    // Jeśli zbyt długo nie ma znaku końca linii to nie ma prawidłowego komunikatu.
    return 0;
}

const int game_buf_size = 548;
const int check_buf_size = 600;
const int word_max_size = 21;

// Dane datagramu od serwera gry.
char game_buffer[game_buf_size];
// Bufor do przechowywania wczytywanego słowa.
char word_buffer[word_max_size];
// Bufor do zapisu bajtów, których crc32 będzie obliczone.
char check_buffer[check_buf_size];

// Ile pól game_buffer zapisano.
int game_max_index = 0;
// Ile bajtów game_buffer odczytano.
int game_index = 0;
// Ile bajtów word_buffer zapisano.
int word_index = 0;
// Ile bajtów check_buffer zapisano.
int check_index = 0;

void reset_game_buffer() {
    game_max_index = 0;
    game_index = 0;
}

// Rozpoczyna zapisywanie kolejno odebranych bajtów do check_buffer.
void start_checking_bytes() {
    check_index = 0;
}

// Zapisuje kolejny bajt do check_buffer.
void save_byte() {
    check_buffer[check_index] = game_buffer[game_index];
    check_index = (check_index + 1) % check_buf_size;
}

// Zwraca true wtedy i tylko wtedy gdy podana wartość jest
// taka sama jak crc32 bajtów z check_buffer.
bool check_crc32(uint32_t val_to_cmp) {
    // Odejmujemy 4 od check_index bo zakładamy, że wczytano już z bufora crc32.
    uint32_t crc32_val = crc32(check_buffer, check_index-4);
    return crc32_val == val_to_cmp;
}

// Wczytuje do word_buffer kolejne n bajtów datagramu.
// Zakładamy, że n < word_max_size.
// Zwraca true tylko w przypadku powodzenia.
bool get_n_bytes(int n, int sock) {
    word_index = 0;
    while (word_index < n) {
        if (game_index >= game_max_index) {
            game_max_index = read(sock, game_buffer, game_buf_size);
            game_index = 0;
            if (game_max_index <= 0) {
                
                game_max_index = 0;
                return false;
            }
        }

        save_byte();
        word_buffer[word_index] = game_buffer[game_index];
        word_index++;
        game_index++;
    }
    return true;
}

// Wczytuje do word_buffer kolejne znaki datagramu do napotkania \0.
// Zwraca true w przypadku powodzenia.
// Po wczytaniu więcej niż 20 znaków lub w przypadku problemów zwraca false.
bool get_word(int sock) {
    word_index = 0;
    while (word_index < 21) {
        if (game_index >= game_max_index) {
            game_max_index = read(sock, game_buffer, game_buf_size);
            game_index = 0;
            if (game_max_index <= 0) {
                game_max_index = 0;
                return false;
            }
        }

        save_byte();

        if (game_buffer[game_index] == 0) {
            word_buffer[word_index] = 0;
            game_index++;
            return true;
        }

        word_buffer[word_index] = game_buffer[game_index];
        word_index++;
        game_index++;
    }
    // Jeśli nie napotkano \0.
    return false;
}

// Zwraca kolejną liczbę 8-bitową z datagramu.
// Ustawia err = true w przypadku błędu.
uint8_t get_uint8(bool& err, int sock) {
    if (get_n_bytes(1, sock)) {
        err = false;
        uint8_t rv;
        memcpy(&rv, word_buffer, 1);
        return rv;
    }
    else {
        err = true; 
        reset_game_buffer();
        return 0;
    }
}

// Zwraca kolejną liczbę 32-bitową z datagramu.
// Ustawia err = true w przypadku błędu.
uint32_t get_uint32(bool& err, int sock) {
    if (get_n_bytes(4, sock)) {
        err = false;
        uint32_t rv;
        memcpy(&rv, word_buffer, 4);
        rv = be32toh(rv);
        return rv;
    }
    else {
        err = true; 
        reset_game_buffer();
        return 0;
    }
}

// Zwraca kolejny napis zakończony \0 z datagramu.
// Ustawia err = true w przypadku błędu.
std::shared_ptr<std::string> get_string(bool& err, int sock) {
    if (get_word(sock)) {
        err = false;
        return std::make_shared<std::string>(word_buffer, strlen(word_buffer));
    }
    else {
        err = true;
        reset_game_buffer();
        return std::make_shared<std::string>();
    }
}

