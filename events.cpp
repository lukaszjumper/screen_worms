#include <string>
#include <endian.h>


#include "game-server.h"
#include "events.h"
#include "game-util.h"

// Tworzy i zwraca zdarzenie o zadanym numerze, typie i rozmiarze.
event *create_event(uint32_t event_no, uint8_t event_type, uint32_t size) {
    event *new_event = new event;
    new_event->size = size;
    new_event->content = new char[new_event->size];

    char *destination = new_event->content;

    // Przekształcenie do sieciowej kolejności bajtów.
    // Odejmujemy 8 od rozmiaru bo nie liczymy rozmiaru pól: len, crc32.
    size = htobe32(size - 8);
    event_no = htobe32(event_no);

    memcpy(destination, &size, 4);
    destination += 4;
    memcpy(destination, &event_no, 4);
    destination += 4;
    memcpy(destination, &event_type, 1);
    destination += 1;

    return new_event;
}

event *create_e_new_game(uint32_t event_no, uint32_t maxx, uint32_t maxy,
                                                         players_set& players) {
    uint32_t size = 0;
    for (auto p = players.begin(); p != players.end(); p++) {
        size += (*p)->name->size() + 1;
    }
    event *new_event = create_event(event_no, 0, size + 21);

    char *destination = new_event->content + 9; // 9 bytów już zapisano.

    // Przekształcenie do sieciowej kolejności bajtów.
    maxx = htobe32(maxx);
    maxy = htobe32(maxy);

    memcpy(destination, &maxx, 4);
    destination += 4;
    memcpy(destination, &maxy, 4);
    destination += 4;

    for (auto p = players.begin(); p != players.end(); p++) {
        memcpy(destination, (*p)->name->c_str(), (*p)->name->size());
        destination += (*p)->name->size();

        destination[0] = 0; // Nazwę kończymy znakiem \0.
        
        destination++;
    }

    const uint32_t crc32_value = htobe32(crc32(new_event->content,
                                                 new_event->size-4));
    memcpy(destination, &crc32_value, 4);

    return new_event;
}

event *create_e_pixel(uint32_t event_no, uint8_t player_no,
                                             uint32_t x, uint32_t y) {
    event *new_event = create_event(event_no, 1, 22);

    char *destination = new_event->content + 9; // 9 bytów już zapisano.

    // Przekształcenie do sieciowej kolejności bajtów.
    x = htobe32(x);
    y = htobe32(y);

    memcpy(destination, &player_no, 1);
    destination += 1;
    memcpy(destination, &x, 4);
    destination += 4;
    memcpy(destination, &y, 4);
    destination += 4;

    const uint32_t crc32_value = htobe32(crc32(new_event->content,
                                                 new_event->size-4));
    memcpy(destination, &crc32_value, 4);

    return new_event;
}

event *create_e_player_eliminated(uint32_t event_no, uint8_t player_no) {
    event *new_event = create_event(event_no, 2, 14);

    char *destination = new_event->content + 9; // 9 bytów już zapisano.
    memcpy(destination, &player_no, 1);
    destination += 1;

    const uint32_t crc32_value = htobe32(crc32(new_event->content,
                                                 new_event->size-4));
    memcpy(destination, &crc32_value, 4);

    return new_event;
}

event *create_e_game_over(uint32_t event_no) {
    event *new_event = create_event(event_no, 3, 13);

    char *destination = new_event->content + 9; // 9 bytów już zapisano.

    const uint32_t crc32_value = htobe32(crc32(new_event->content,
                                                 new_event->size-4));
    memcpy(destination, &crc32_value, 4);

    return new_event;
}

void destroy_event(event *event_to_destroy) {
    delete[] event_to_destroy->content;
    delete event_to_destroy;
}