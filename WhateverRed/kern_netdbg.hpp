//  Copyright © 2022 ChefKiss Inc. Licensed under the Non-Profit Open Software License version 3.0. See LICENSE for
//  details.

#pragma once
#include <cstdarg>
#include <sys/socket.h>
#include <sys/types.h>

#define NETLOG(mod, fmt, ...) NETDBG::printf(mod ": " fmt "\n", ##__VA_ARGS__)

class NETDBG {
    static in_addr_t ip_addr;
    static uint32_t port;

    public:
    static bool enabled;
    static size_t nprint(char *data, size_t len);
    [[gnu::format(__printf__, 1, 2)]] static size_t printf(const char *fmt, ...);
    [[gnu::format(__printf__, 1, 0)]] static size_t vprintf(const char *fmt, va_list args);
};
