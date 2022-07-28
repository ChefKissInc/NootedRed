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

class NETDBG
{
public:
	[[gnu::format(__printf__, 1, 2)]] static size_t sendData(const char* fmt, ...);
};

#endif /* kern_netdbg_hpp */
