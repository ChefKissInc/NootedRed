// std::enable_if ports
//
// Copyright (C) 2025-2026  ChefKiss.
//
// Licensed under the ChefKiss Open Knowledge License version 1.
//
// This work is licensed to you under the terms of the ChefKiss Open Knowledge
// License, either version 1 of the License, or (at your option) any later
// version published by ChefKiss.
//
// This work may not be used to train or develop artificial intelligence,
// and may not be used with artificial intelligence in any other way. See
// the ChefKiss Open Knowledge License for details.
//
// This work is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ChefKiss Open Knowledge License for more details.
//
// You should have received a copy of the ChefKiss Open Knowledge License
// along with this work.  If not, see https://git.chefkiss.dev/ChefKiss/OpenKnowledge/raw/branch/master/LICENSE-v1.md.

#pragma once

template<typename T>
struct is_void
{
    static constexpr bool value = false;
};

template<>
struct is_void<void>
{
    static constexpr bool value = true;
};

template<typename T>
static constexpr bool is_void_v = is_void<T>::value;

template<bool B, typename T = void>
struct enable_if
{ };

template<typename T>
struct enable_if<true, T>
{
    using type = T;
};

template<bool B, typename T>
using enable_if_t = typename enable_if<B, T>::type;
