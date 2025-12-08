#include "../include/websockets.hpp"

// helper for encoding base64
std::string base64_encode(const unsigned char *data, size_t len)
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *mem = BIO_new(BIO_s_mem());
    BIO_push(b64, mem);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data, static_cast<int>(len));
    BIO_flush(b64);

    BUF_MEM *memPtr;
    BIO_get_mem_ptr(b64, &memPtr);

    std::string out(memPtr->data, memPtr->length);

    BIO_free_all(b64);
    return out;
}

bool is_websocket_upgrade(const std::string &raw, std::string &secKeyOut)
{
    std::istringstream iss(raw);
    std::string line;

    bool hasUpgrade = false;
    bool hasConnection = false;
    std::string secKey;

    // skip request line, start from headers
    std::getline(iss, line);

    while (std::getline(iss, line))
    {
        if (line == "\r" || line.empty())
            break;

        auto pos = line.find(':');
        if (pos == std::string::npos)
            continue;
        std::string name = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        // trim
        auto trim = [](std::string &s)
        {
            while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
                s.pop_back();
            size_t i = 0;
            while (i < s.size() && (s[i] == ' ' || s[i] == '\t'))
                ++i;
            s.erase(0, i);
        };
        trim(name);
        trim(value);

        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        if (lowerName == "upgrade" && value.find("websocket") != std::string::npos)
        {
            hasUpgrade = true;
        }
        if (lowerName == "connection" && value.find("Upgrade") != std::string::npos)
        {
            hasConnection = true;
        }
        if (lowerName == "sec-websocket-key")
        {
            secKey = value;
        }
    }

    if (hasUpgrade && hasConnection && !secKey.empty())
    {
        secKeyOut = secKey;
        return true;
    }
    return false;
}

std::string compute_websocket_accept(const std::string &secKey)
{
    static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string toHash = secKey + GUID;

    unsigned char sha1[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(toHash.data()), toHash.size(), sha1);

    return base64_encode(sha1, SHA_DIGEST_LENGTH);
}

bool recv_ws_frame(int fd, std::string &outPayload)
{
    uint8_t header[2];
    int n = recv(fd, header, 2, 0);
    if (n <= 0)
        return false;

    [[maybe_unused]] bool fin = (header[0] & 0x80) != 0;
    uint8_t opcode = header[0] & 0x0F;
    bool mask = (header[1] & 0x80) != 0;
    uint64_t len = header[1] & 0x7F;

    if (!mask)
    {
        // Client frames MUST be masked
        return false;
    }

    if (len == 126)
    {
        uint8_t ext[2];
        n = recv(fd, ext, 2, 0);
        if (n <= 0)
            return false;
        len = (ext[0] << 8) | ext[1];
    }
    else if (len == 127)
    {
        // skipping 64-bit payloads for brevity
        return false;
    }

    uint8_t maskKey[4];
    n = recv(fd, maskKey, 4, 0);
    if (n <= 0)
        return false;

    std::string payload(len, '\0');
    size_t received = 0;
    while (received < len)
    {
        n = recv(fd, &payload[received], len - received, 0);
        if (n <= 0)
            return false;
        received += n;
    }

    for (uint64_t i = 0; i < len; ++i)
    {
        payload[i] ^= maskKey[i % 4];
    }

    if (opcode == 0x8)
    {
        // close frame
        return false;
    }
    if (opcode == 0x1)
    {
        // text frame
        outPayload = payload;
        return true;
    }

    // ignore other opcodes
    return true;
}

void send_ws_text(int fd, const std::string &msg)
{
    uint8_t header[10];
    size_t len = msg.size();
    size_t headerLen = 0;

    header[0] = 0x81; // FIN=1, opcode=1 (text)

    if (len <= 125)
    {
        header[1] = static_cast<uint8_t>(len);
        headerLen = 2;
    }
    else if (len < 65536)
    {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        headerLen = 4;
    }
    else
    {
        // large frames not supported in this minimal implementation
        return;
    }

    send(fd, header, headerLen, 0);
    send(fd, msg.data(), msg.size(), 0);
}