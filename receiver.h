#ifndef __RECEIVER_H__
#define __RECEIVER_H__

#include <netinet/in.h>
#include <memory>
#include <cstring>

#include "types.h"


class Receiver {

private:
    const int flags = MSG_WAITALL;

    int sock;
    std::shared_ptr<client_to_server> cts;
    std::shared_ptr<sockaddr_in6> client_address;
    socklen_t rcva_len;
    int name_size();
    bool validate(int len);
public:
    Receiver(int socket);
    bool next_receive();
    uint64_t get_id();
    uint8_t get_direction();
    uint32_t get_next_expected_event_no();
    in_port_t get_port_nr();
    std::shared_ptr<sockaddr_in6> get_address();
    std::shared_ptr<std::string> get_ip();
    std::shared_ptr<std::string> get_name();
};

#endif /* __RECEIVER_H__ */