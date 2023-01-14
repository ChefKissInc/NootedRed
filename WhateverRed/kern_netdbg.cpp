//
//  kern_netdbg.cpp
//  WhateverRed
//
//  Created by Nyan Cat on 7/27/22.
//  Copyright © 2022 ChefKiss Inc. All rights reserved.
//

#include "kern_netdbg.hpp"
#include <Headers/kern_api.hpp>
#include <netinet/in.h>

in_addr_t inet_addr(uint32_t a, uint32_t b, uint32_t c, uint32_t d) { return a | (b << 8) | (c << 16) | (d << 24); }

bool NETDBG::enabled = false;
in_addr_t NETDBG::ip_addr = 0;
uint32_t NETDBG::port = 0;

size_t NETDBG::nprint(char *data, size_t len) {
    if (enabled && PE_parse_boot_argn("wrednetdbg", &enabled, sizeof(enabled) && !enabled)) {
        kprintf("netdbg: Disabled via boot arg\n");
        enabled = false;
    }

    kprintf("%s", data);

    if (!enabled) { return 0; }

    if (!ip_addr || !port) {
        uint8_t b[4] = {0};
        uint32_t p = 0;
        char ip[128];
        if (!PE_parse_boot_argn("wrednetdbgip", &ip, sizeof(ip))) { panic("netdbg: No IP specified"); }

        if (sscanf(ip, "%hhu.%hhu.%hhu.%hhu:%u", &b[0], &b[1], &b[2], &b[3], &p) != 5) {
            panic("netdbg: Invalid IP and/or Port specified");
        }

        ip_addr = inet_addr(b[0], b[1], b[2], b[3]);
        port = htons(p);

        if (!ip_addr || !port) { panic("netdbg: Invalid IP and/or Port specified"); }
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
    char *data = new char[2048];
    size_t len = vsnprintf(data, 2047, fmt, args);

    auto ret = NETDBG::nprint(data, len);

    delete[] data;
    return ret;
}
