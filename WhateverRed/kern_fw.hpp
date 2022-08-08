//
//  kern_fw.hpp
//  WhateverRed
//
//  Created by Visual on 22/7/22.
//  Copyright © 2022 VisualDevelopment. All rights reserved.
//

#ifndef kern_fw_h
#define kern_fw_h
#include <libkern/c++/OSData.h>
#include <string.h>

struct FwDesc {
    const char *name;
    const unsigned char *var;
    const int size;
};

#define RAD_FW(fw_name, fw_var, fw_size) \
    .name = fw_name, .var = fw_var, .size = fw_size

extern const struct FwDesc fwList[];
extern const int fwNumber;

static inline OSData *getFWDescByName(const char *name) {
    for (int i = 0; i < fwNumber; i++) {
        if (strcmp(fwList[i].name, name) == 0) {
            FwDesc desc = fwList[i];
            return OSData::withBytes(desc.var, desc.size);
        }
    }
    return NULL;
}

#endif /* kern_fw_h */
