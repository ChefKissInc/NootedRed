#!/usr/bin/python3

# Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
# See LICENSE for details.

import os
import sys


def is_file_excluded(name: str) -> bool:
    return name.startswith(".") or name == "LICENSE"


def is_file_text(name: str) -> bool:
    return not name.endswith(".dat") and not name.endswith(".bin")


def process_files(target_file, dir):
    os.makedirs(os.path.dirname(target_file), exist_ok=True)
    lines = ['#include "PrivateHeaders/Firmware.hpp"', ""]
    file_list = []
    files = filter(
        lambda v: not is_file_excluded(os.path.basename(v[1])),
        [(root, file) for root, _, files in os.walk(dir) for file in files],
    )
    for root, file in files:
        is_text = is_file_text(os.path.basename(file))
        var_ident = file.replace(".", "_").replace("-", "_")
        lines += [
            f"static const UInt8 {var_ident}[] = {{",
            f'#embed "{os.path.join(root, file)}"',
        ]
        if is_text:
            lines += [r", '\0'"]
        lines += ["};"]
        file_list += [
            f'    {{.name = "{file}", .metadata = {{.data = {var_ident}, .length = sizeof({var_ident})}}}}'
        ]

    lines += ["", "const struct FWDescriptor firmware[] = {"]
    lines += file_list
    lines += ["};", "", f"const size_t firmwareCount = {len(file_list)};"]

    with open(target_file, "w") as file:
        file.writelines(map(lambda v: v + "\n", lines))


if __name__ == "__main__":
    process_files(sys.argv[1], sys.argv[2])
