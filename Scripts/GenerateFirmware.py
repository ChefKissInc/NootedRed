#!/usr/bin/python3

# Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
# See LICENSE for details.

import os
import sys

header = """#include "Firmware.hpp"

#define A(N, D) static const UInt8 N[] = D 
#define F(N, D, L) {.name = N, .metadata = {.data = D, .length = L}}
"""

special_chars = {
    0x0: "\\0",
    0x7: "\\a",
    0x8: "\\b",
    0x9: "\\t",
    0xA: "\\n",
    0xB: "\\v",
    0xC: "\\f",
    0xD: "\\r",
}
symbols = b" !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
needs_escape = b'"\\'


def byte_to_char(b, is_text: bool):
    if b in special_chars:
        return special_chars[b]
    elif b in symbols:
        if b in needs_escape:
            return "\\" + chr(b)
        else:
            return chr(b)
    elif is_text:
        if 0 <= b <= 127 and chr(b).isalnum():
            return chr(b)
        else:
            assert False
    else:
        return f"\\x{b:X}"


def bytes_to_cstr(data, is_text=False):
    return '"' + "".join(byte_to_char(b, is_text) for b in data) + '"'


def is_file_excluded(name: str) -> bool:
    return name.startswith(".") or name == "LICENSE"


def is_file_text(name: str) -> bool:
    return not name.endswith(".dat") and not name.endswith(".bin")


def process_files(target_file, dir):
    os.makedirs(os.path.dirname(target_file), exist_ok=True)
    lines = header.splitlines(keepends=True) + ["\n"]
    file_list_content = []
    files = filter(
        lambda v: not is_file_excluded(os.path.basename(v[1])),
        [(root, file) for root, _, files in os.walk(dir) for file in files],
    )
    for root, file in files:
        with open(os.path.join(root, file), "rb") as src_file:
            src_data = src_file.read()
            src_len = len(src_data)
        is_text = is_file_text(os.path.basename(file))
        var_ident = file.replace(".", "_").replace("-", "_")
        var_contents = bytes_to_cstr(src_data, is_text)
        lines.append(f"A({var_ident}, {var_contents});\n")
        var_len = src_len + 1 if is_text else src_len  # NUL Byte
        file_list_content.append(f'    F("{file}", {var_ident}, 0x{var_len:X}),\n')

    lines.append("\nconst struct FWDescriptor firmware[] = {\n")
    lines += file_list_content
    lines += ["};\n", f"const size_t firmwareCount = {len(file_list_content)};\n"]

    with open(target_file, "w") as file:
        file.writelines(lines)


if __name__ == "__main__":
    process_files(sys.argv[1], sys.argv[2])
