//
//  kern_netdbg.cpp
//  WhateverRed
//
//  Created by Nyan Cat on 7/27/22.
//  Copyright © 2022 VisualDevelopment. All rights reserved.
//

#include "kern_netdbg.hpp"
#include <Headers/kern_api.hpp>
#include <netinet/in.h>

in_addr_t inet_addr(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	auto ret = d;
	
	ret *= 256;
	ret += c;
	
	ret *= 256;
	ret += b;
	
	ret *= 256;
	ret += a;
	
	return ret;
}

size_t NETDBG::nprint(char *data, size_t len)
{
	SYSLOG("netdbg", "message: %s", data);
	int retry = 5;
	while (retry--) {
		socket_t socket = nullptr;
		sock_socket(AF_INET, SOCK_STREAM, 0, NULL, 0, &socket);
		SYSLOG("rad", "sendData socket=%d", socket);
		if (!socket) return false;
		
		struct sockaddr_in info;
		bzero(&info, sizeof(info));
		
		info.sin_len = sizeof(sockaddr_in);
		info.sin_family = PF_INET;
		info.sin_addr.s_addr = inet_addr(149, 102, 131, 82);
		info.sin_port = htons(420);
		
		uint32_t timeout = 10'000;
		sock_setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

		int err = sock_connect(socket, (sockaddr *)&info, 0);
		SYSLOG("rad", "sendData err=%d", err);
		if (err == -1) {
			sock_close(socket);
			continue;
		}
		
		iovec vec { .iov_base = data, .iov_len = len };
		msghdr hdr {
			.msg_iov = &vec,
			.msg_iovlen = 1,
		};
		
		size_t sentLen = 0;
		sock_send(socket, &hdr, 0, &sentLen);
		
		SYSLOG("rad", "sendData sentLen=%d", sentLen);
		sock_close(socket);
		
		return sentLen;
	}
	
	return 0;
}

size_t NETDBG::printf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	auto ret = NETDBG::vprintf(fmt, args);
	va_end(args);
	return ret;
}

size_t NETDBG::vprintf(const char *fmt, va_list args)
{
	char *data = new char[1024];
	size_t len = vsnprintf(data, 1024, fmt, args);
	
	auto ret = NETDBG::nprint(data, len);
	delete[] data;
	return ret;
}
