#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <limits>
#include <getopt.h>
#include <endian.h>

#include "types.h"
#include "game-client.h"



void failure(const char *info) {
    std::cerr << info << "\n";
    exit(EXIT_FAILURE);
}

void usage() {
    failure("Użycie: ./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]");
}

int main(int argc, char *argv[]) {
    long long optval;
    int opt;

    if (argc < 2) {
        usage();
    }

    char game_server_port[6];
    strcpy(game_server_port, "2021");
    char interface_server_port[6];
    strcpy(interface_server_port, "20210");
    char player_name[21];
    strcpy(player_name, "");

    char gui_server[50];
    char game_server[50];
    
    strcpy(gui_server, "localhost");
    strncpy(game_server, argv[1], 49);

    while ((opt = getopt(argc, argv, "n:p:i:r:")) != -1) {
        switch (opt) {
            case 'p':
                optval = atoll(optarg);
                if (optval < 0 || optval > std::numeric_limits<in_port_t>::max()) {
                    failure("Niepoprawny parametr -p");
                }
                memset(game_server_port, 0, 6);
                strncpy(game_server_port, optarg, 5);
                break;
            case 'r':
                memset(interface_server_port, 0, 6);
                strncpy(interface_server_port, optarg, 5);
                break;
            case 'n':
                strncpy(player_name, optarg, 20);
                break;
            case 'i':
                strncpy(gui_server, optarg, 49);
                break;
            default:
                usage();
                break;
        }
    }

    if (optind + 1 < argc) {
        failure("Znaleziono niepoprawny argument.");
    }

    // Poprawność nazwy gracza.
    for (int i = 0; i < 21; i++) {
        if (player_name[i] == 0) {
            break;
        }
        if (i == 20) {
            failure("Zbyt długa nazwa gracza.");
        }
        if (player_name[i] < 33 || player_name[i] > 126) {
            failure("Nazwa gracza zawiera niedozwolone znaki.");
        }
    }

    // Połączenie z serwerem gry.

    int val;
    int sock_game, sock_interface;

    addrinfo addr_hints;
    addrinfo *addr_result;

    memset(&addr_hints, 0, sizeof(addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(game_server, game_server_port, &addr_hints, &addr_result) != 0) {
        failure("Błąd podczas wywołania getaddrinfo");
    }
        
    sock_game = socket(addr_result->ai_family,
                         addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock_game < 0) {
        failure("Błąd podczas tworzenia gniazda.");
    }

    // Timeout dla odbierania i wysyłania.
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;
    if (setsockopt(sock_game, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, 
           sizeof(timeout))) {
        failure("Błąd podczas ustawiania opcji gniazda.");
    }
    if (setsockopt(sock_game, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, 
           sizeof(timeout))) {
        failure("Błąd podczas ustawiania opcji gniazda.");
    }

    if (connect(sock_game, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
        failure("Błąd połączenia z serwerem gry.");
    }

    freeaddrinfo(addr_result);


    // Połączenie z interfejsem użytkownika.
    
    memset(&addr_hints, 0, sizeof(addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_protocol = IPPROTO_TCP;
    addr_hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(gui_server, interface_server_port, &addr_hints, &addr_result) != 0) {
        failure("Błąd podczas wywołania getaddrinfo.");
    }

    sock_interface = socket(addr_result->ai_family,
                         addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock_interface < 0) {
        failure("Błąd podczas otwierania gniazda.");
    }

    // Ustawiamy timeout
    if (setsockopt(sock_interface, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, 
           sizeof(timeout))) {
        failure("Błąd podczas ustawiania opcji gniazda.");
    }
    if (setsockopt(sock_interface, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, 
           sizeof(timeout))) {
        failure("Błąd podczas ustawiania opcji gniazda.");
    }
    // Wyłączamy algorytm Nagle'a.
    val = 1;
    if (setsockopt(sock_interface, IPPROTO_TCP, TCP_NODELAY, (void*)&val, sizeof(val)) < 0) {
        failure("Błąd podczas ustawiania opcji gniazda.");
    }

    if (connect(sock_interface, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
        failure("Błąd połączenia z interfejsem użytkownika.");
    }

    freeaddrinfo(addr_result);

    // Uruchomienie gry.

    Client client(sock_game, sock_interface, player_name, strlen(player_name));
    client.start();

    // Sprzątanie.

    close(sock_game);
    close(sock_interface);

    return 0;
}