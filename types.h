#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <map>
#include <set>

struct client_to_server {
    uint64_t session_id;
    uint8_t turn_direction;
    uint32_t next_expected_event_no;
};

// Informacje o graczu w danej partii.
struct player {
    std::shared_ptr<std::string> name;
    uint8_t direction_change;
    long double direction;
    long double x;
    long double y;
    bool is_playing;
    uint8_t number; // Którym graczem na liście jest ten.
};

struct client_opt {
    std::shared_ptr<sockaddr_in6> address;
    uint64_t last_request;
    uint64_t id;
    std::shared_ptr<player> this_player;
    bool clicked_sth;
    bool accepted; // Czy serwer pozwolił brać czynny udział w grze.
};

struct event {
    uint32_t size;
    char* content;
};


#endif /* __TYPES_H__ */