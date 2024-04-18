#!/usr/bin/python3

import os
import struct
import sys

header = '''
//! Copyright © 2022-2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "Firmware.hpp"
'''


def format_file_name(file_name):
    return file_name.replace(".", "_").replace("-", "_")


def lines_for_file(path, file):
    with open(path, "rb") as src_file:
        src_data = src_file.read()
        src_len = len(src_data)

    lines: list[str] = []
    fw_var_name = format_file_name(file)
    lines.append(f"\nconst unsigned char {fw_var_name}[] = {{\n")
    index = 0
    block = []
    while index < src_len:
        block = src_data[index:src_len if index +
                         16 >= src_len else index + 16]
        index += 16

        if len(block) < 16:
            lines.append(
                f"    {', '.join(f'0x{b:X}' for b in block)}\n")
        else:
            lines.append("    0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, "
                         "0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X},\n"
                         .format(*struct.unpack("BBBBBBBBBBBBBBBB", block)))
    return lines + [
        "};\n",
        f"const UInt32 {fw_var_name}_size = sizeof({fw_var_name});\n",
    ]


def process_files(target_file, dir):
    os.makedirs(os.path.dirname(target_file), exist_ok=True)
    lines: list[str] = header.splitlines(keepends=True)
    files = list(filter(lambda v: not os.path.basename(v[1]).startswith('.') and not os.path.basename(v[1]) == "LICENSE", [
        (root, file) for root, _, files in os.walk(dir) for file in files]))
    file_list_content: list[str] = []
    for root, file in files:
        lines += lines_for_file(os.path.join(root, file), file)
        fw_var_name = format_file_name(file)
        file_list_content += [
            f"    {{FIRMWARE(\"{file}\", {fw_var_name}, {fw_var_name}_size)}},\n"]

    lines += ["\n", "const struct FWDescriptor firmware[] = {\n"]
    lines += file_list_content
    lines += ["};\n", f"const size_t firmwareCount = {len(files)};\n"]

    with open(target_file, "w") as file:
        file.writelines(lines)


if __name__ == '__main__':
    process_files(sys.argv[1], sys.argv[2])
