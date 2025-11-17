// std::is_function port
//
// Copyright © 2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
// See LICENSE for details.

#pragma once
template<typename T>
struct is_function {
    static constexpr bool value = false;
};

template<typename R, typename... Args>
struct is_function<R(Args...)> {
    static constexpr bool value = true;
};

template<typename T>
static constexpr bool is_function_v = is_function<T>::value;
