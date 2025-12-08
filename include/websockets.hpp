#pragma once
#include <string>
#include <cstdint>
#include <sys/socket.h>
#include <openssl/types.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <sstream>
#include <algorithm>


// Handshake helpers
bool is_websocket_upgrade(const std::string& raw, std::string& secKeyOut);
std::string compute_websocket_accept(const std::string& secKey);

// Frame helpers
bool recv_ws_frame(int fd, std::string& outPayload); 
void send_ws_text(int fd, const std::string& msg);     