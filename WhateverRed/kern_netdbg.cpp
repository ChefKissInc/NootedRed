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

size_t NETDBG::nprint(char *data, size_t len) {
    kprintf("netdbg: message: %s", data);

    if (!enabled) {
        return 0;
    } else if (!socket) {
        int retry = 5;
        while (retry--) {
            if (!socket) { sock_socket(AF_INET, SOCK_STREAM, 0, NULL, 0, &socket); }
            if (!socket) { continue; }

            struct sockaddr_in info;
            bzero(&info, sizeof(info));

            info.sin_len = sizeof(sockaddr_in);
            info.sin_family = PF_INET;
            info.sin_addr.s_addr = inet_addr(38, 242, 250, 111);
            info.sin_port = htons(420);

            int err = sock_connect(socket, (sockaddr *)&info, 0);
            if (err == -1) {
                SYSLOG("netdbg", "nprint err=%d", err);
                sock_close(socket);
                socket = nullptr;
                continue;
            }
        }

        if (!socket || !retry) {
            socket = nullptr;
            return 0;
        }
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
