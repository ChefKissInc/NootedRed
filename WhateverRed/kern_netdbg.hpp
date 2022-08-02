//
//  kern_netdbg.hpp
//  WhateverRed
//
//  Created by Nyan Cat on 7/27/22.
//  Copyright © 2022 VisualDevelopment. All rights reserved.
//

#ifndef kern_netdbg_hpp
#define kern_netdbg_hpp
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdarg>

class NETDBG
{
public:
	static bool enabled;
	static socket_t socket;
	static size_t nprint(char *data, size_t len);
	[[gnu::format(__printf__, 1, 2)]] static size_t printf(const char* fmt, ...);
	[[gnu::format(__printf__, 1, 0)]] static size_t vprintf(const char* fmt, va_list args);
};

#endif /* kern_netdbg_hpp */
