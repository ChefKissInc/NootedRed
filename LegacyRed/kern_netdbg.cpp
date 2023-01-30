//  Copyright © 2022 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_netdbg.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <IOKit/IOLocks.h>
#include <netinet/in.h>

in_addr_t NETDBG::ip_addr = 0;
uint32_t NETDBG::port = 0;
bool NETDBG::enabled = false;

size_t NETDBG::nprint(char *data, size_t len) {
    if (enabled && PE_parse_boot_argn("wrednetdbg", &enabled, sizeof(enabled) && !enabled)) {
        SYSLOG("netdbg", "Disabled via boot arg");
        enabled = false;
    }

    kprintf("%s", data);

    if (!enabled) { return 0; }

    if (!ip_addr || !port) {
        uint8_t ipParts[4] = {0};
        uint32_t port = 0;
        char ip[64];
        PANIC_COND(!PE_parse_boot_argn("wrednetdbgip", &ip, sizeof(ip)), "netdbg", "No IP specified");
        PANIC_COND(sscanf(ip, "%hhu.%hhu.%hhu.%hhu:%u", &ipParts[0], &ipParts[1], &ipParts[2], &ipParts[3], &port) != 5,
            "netdbg", "Invalid IP and/or Port specified");

        ip_addr = ipParts[0] | (ipParts[1] << 8) | (ipParts[2] << 16) | (ipParts[3] << 24);
        port = htons(port);

        PANIC_COND(!ip_addr || !port, "netdbg", "Invalid IP and/or Port specified");
    }

    socket_t socket = nullptr;
    int retry = 5;
    while (retry--) {
        if (!socket) { sock_socket(AF_INET, SOCK_STREAM, 0, NULL, 0, &socket); }
        if (!socket) { continue; }

        struct sockaddr_in info;
        bzero(&info, sizeof(info));

        info.sin_len = sizeof(sockaddr_in);
        info.sin_family = PF_INET;
        info.sin_addr.s_addr = ip_addr;
        info.sin_port = port;

        int err = sock_connect(socket, (sockaddr *)&info, 0);
        if (err == -1) {
            SYSLOG("netdbg", "nprint err=%d", err);
            sock_close(socket);
            socket = nullptr;
            continue;
        }
    }

    if (!socket || (!retry && !socket)) { return 0; }
    if (!retry) {
        sock_close(socket);
        return 0;
    }

    iovec vec {.iov_base = data, .iov_len = len};
    msghdr hdr {
        .msg_iov = &vec,
        .msg_iovlen = 1,
    };

    size_t sentLen = 0;
    int err = sock_send(socket, &hdr, 0, &sentLen);
    if (err == -1) {
        SYSLOG("netdbg", "nprint err=%d", err);
        sock_close(socket);
        return 0;
    }
    sock_close(socket);

    return sentLen;
}

size_t NETDBG::printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    auto ret = NETDBG::vprintf(fmt, args);
    va_end(args);
    return ret;
}

size_t NETDBG::vprintf(const char *fmt, va_list args) {
    static IOLock *lock = nullptr;
    if (lock == nullptr) lock = IOLockAlloc();
    IOLockLock(lock);
    char *data = new char[2048];
    size_t len = vsnprintf(data, 2047, fmt, args);

    auto ret = NETDBG::nprint(data, len);

    delete[] data;
    IOLockUnlock(lock);
    return ret;
}