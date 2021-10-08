#include <endian.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>

#include "receiver.h"

const int buffer_size = 35;

char buffer[buffer_size];
char name[20]; // Przechowujemy tu nazwę klienta od którego ostatnio coś odebrano.

Receiver::Receiver(int socket) {
    sock = socket;
    cts = std::make_shared<client_to_server>();
}

// Odbiera komunikat od klienta i przetwarza razem z informacją o kliencie.
// Zwraca true wtedy i tylko wtedy gdy nastąpił sukces.
bool Receiver::next_receive() {
    client_address = std::make_shared<sockaddr_in6>();
    rcva_len = sizeof(*client_address.get());
    memset(name, 0, 20); // Zerujemy nazwę.

    int len = recvfrom(sock, buffer, buffer_size, flags,
		     (struct sockaddr *) client_address.get(), &rcva_len);

    if (len < 13 || len > 33) {
        return false;
    }

    memcpy(&cts->session_id, buffer, 8);
    memcpy(&cts->turn_direction, buffer+8, 1);
    memcpy(&cts->next_expected_event_no, buffer+9, 4);
    memcpy(name, buffer+13, len-13);

    cts->session_id = be64toh(cts->session_id);
    cts->next_expected_event_no = be32toh(cts->next_expected_event_no);

    return validate(len - 13);
}

// Sprawdza czy otrzymana wiadomość ma poprawny format.
// Zwraca true wtedy i tylko wtedy gdy nie napotka błędów.
// len to długość nazwy gracza.
bool Receiver::validate(int len) {
    if (cts->turn_direction >= 3) {
        return false;
    }
    // Sprawdzamy nazwę gracza
    if (len > 20) {
        return false;
    }
    for (int i = 0; i < len; i++) {
        if (name[i] < 33 || name[i] > 126) {
            return false;
        }
    }
    return true;
}

uint64_t Receiver::get_id() {
    return cts->session_id;
} 

uint8_t Receiver::get_direction() {
    return cts->turn_direction;
}

uint32_t Receiver::get_next_expected_event_no() {
    return cts->next_expected_event_no;
}

in_port_t Receiver::get_port_nr() {
    return client_address->sin6_port;
}

std::shared_ptr<sockaddr_in6> Receiver::get_address() {
    return client_address;
}

std::shared_ptr<std::string> Receiver::get_name() {
    return std::make_shared<std::string>(name, strnlen(name, 20));
}

std::shared_ptr<std::string> Receiver::get_ip() {
    auto str_ptr = std::make_shared<std::string>(INET6_ADDRSTRLEN+1, 0);
    inet_ntop(AF_INET6, &client_address->sin6_addr, (char *) str_ptr->c_str(),
                 INET6_ADDRSTRLEN+1);
    return str_ptr;
}