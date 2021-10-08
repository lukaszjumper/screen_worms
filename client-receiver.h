#ifndef __CLIENT_RECEIVER_H__
#define __CLIENT_RECEIVER_H__

#include <memory>

int interface_next(int sock);
uint8_t get_uint8(bool& err, int sock);
uint32_t get_uint32(bool& err, int sock);
std::shared_ptr<std::string> get_string(bool& err, int sock);
void reset_game_buffer();
void start_checking_bytes();
bool check_crc32(uint32_t val_to_cmp);

#endif /* __CLIENT_RECEIVER_H__ */