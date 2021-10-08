#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <limits>

#include "receiver.h"
#include "game-server.h"

// Ograniczenia.
const int max_turning_speed = 90;
const int max_rounds_per_sec = 250;
const int max_width = 4096;
const int max_height = 4096;

void failure(const char *info) {
    std::cerr << info << "\n";
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int opt;
    long long optval;
    // Domyślne parametry.
    int port_nr = 2021;
    uint32_t seed = time(NULL);
    int turning_speed = 6;
    int rounds_per_sec = 50;
    int width = 640;
    int height = 480;

    while ((opt = getopt(argc, argv, "p:s:t:v:w:h:")) != -1) {
        switch (opt) {
            case 'p':
                optval = atoll(optarg);
                if (optval <= 0 || optval > std::numeric_limits<in_port_t>::max()) {
                    failure("Niepoprawny parametr -p");
                }
                port_nr = optval;
                break;
            case 's':
                optval = atoll(optarg);
                if (optval <= 0 || optval > std::numeric_limits<uint32_t>::max()) {
                    failure("Niepoprawny parametr -s");
                }
                seed = optval;
                break;
            case 't':
                optval = atoll(optarg);
                if (optval <= 0 || optval > max_turning_speed) {
                    failure("Niepoprawny parametr -t");
                }
                turning_speed = optval;
                break;
            case 'v':
                optval = atoll(optarg);
                if (optval <= 0 || optval > max_rounds_per_sec) {
                    failure("Niepoprawny parametr -v");
                }
                rounds_per_sec = optval;
                break;
            case 'w':
                optval = atoll(optarg);
                if (optval <= 0 || optval > max_width) {
                    failure("Niepoprawny parametr -w");
                }
                width = optval;
                break;
            case 'h':
                optval = atoll(optarg);
                if (optval <= 0 || optval > max_height) {
                    failure("Niepoprawny parametr -h");
                }
                height = optval;
                break;
            default:
                failure("Użycie: ./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]");
        }
    }

    if (optind < argc) {
        failure("Znaleziono niepoprawny argument.");
    } 

    
    // Tworzenie ganiazda.
    int sock;
    sockaddr_in6 server_address;

    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        failure("Błąd podczas tworzenia gniazda.");
    }
    
    // Niekoniecznie używamy tylko IPv6.
    int val = 0;
    if (setsockopt(sock, SOL_IPV6, IPV6_V6ONLY, (void*)&val, sizeof(val)) < 0) {
        failure("Błąd podczas ustawiania opcji gniazda.");
    }

    // Timeout dla odbierania i wysyłania.
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, 
           sizeof(timeout))) {
        failure("Błąd podczas ustawiania opcji gniazda.");
    }
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, 
           sizeof(timeout))) {
        failure("Błąd podczas ustawiania opcji gniazda.");
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin6_family = AF_INET6;
    server_address.sin6_flowinfo = 0;
    server_address.sin6_addr = in6addr_any;
    server_address.sin6_port = htons(port_nr);
    if (bind(sock, (sockaddr *)&server_address, sizeof(server_address)) < 0) {
        failure("Błąd podczas wywołania bind.");
    }

    // Uruchomienie gry.
    Server server(turning_speed, rounds_per_sec, width, height, sock, seed);
    server.start();

    close(sock);
    return 0;
}