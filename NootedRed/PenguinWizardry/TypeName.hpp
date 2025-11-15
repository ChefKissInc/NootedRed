// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once

template<typename T>
const char *type_name();

#define DEFINE_TYPE_NAME(_ty)      \
    template<>                     \
    const char *type_name<_ty>() { \
        return #_ty;               \
    }
