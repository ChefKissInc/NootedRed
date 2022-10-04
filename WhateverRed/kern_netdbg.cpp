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

in_addr_t inet_addr(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    auto ret = d;

    ret *= 256;
    ret += c;

    ret *= 256;
    ret += b;

    ret *= 256;
    ret += a;

    return ret;
}

bool NETDBG::enabled = false;
socket_t NETDBG::socket = nullptr;
in_addr_t NETDBG::ip_addr = 0;
uint32_t NETDBG::port = 0;

size_t NETDBG::nprint(char *data, size_t len) {
    kprintf("netdbg: message: %s", data);

    if (!enabled) { return 0; }

    bool disable = false;
    if (PE_parse_boot_argn("netdbg_disable", &disable, sizeof(bool)) && disable) {
        kprintf("netdbg: Disabled via boot arg");
        enabled = false;
        return 0;
    }

    if (!ip_addr || !port) {
        uint32_t b[4] = {0};
        uint32_t p = 0;
        if (!PE_parse_boot_argn("netdbg_ip_1", b, sizeof(uint32_t)) ||
            !PE_parse_boot_argn("netdbg_ip_2", b + 1, sizeof(uint32_t)) ||
            !PE_parse_boot_argn("netdbg_ip_3", b + 2, sizeof(uint32_t)) ||
            !PE_parse_boot_argn("netdbg_ip_4", b + 3, sizeof(uint32_t)) ||
            !PE_parse_boot_argn("netdbg_port", &p, sizeof(uint32_t))) {
            panic("netdbg: No IP and/or Port specified");
            return 0;
        }

        ip_addr = inet_addr(b[0], b[1], b[2], b[3]);
        port = htons(p);

        if (!ip_addr || !port) {
            panic("netdbg: Invalid IP and/or Port specified");
            return 0;
        }
    }

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
        socket = nullptr;
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
        socket = nullptr;
        return 0;
    }

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
