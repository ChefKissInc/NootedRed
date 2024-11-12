#!/usr/bin/python3

# Copyright © 2022-2024 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
# See LICENSE for details.

import os
import sys

header = """#include <Headers/kern_util.hpp>
#include <PrivateHeaders/Firmware.hpp>

#define A(N, D) static const UInt8 N[] = D 
#define F(N, D, L) {.name = N, .metadata = {.data = D, .length = L}}
"""

special_chars = {
    0x0: "\\0",
    0x7: "\\a",
    0x8: "\\b",
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
    elif b == 0x9:
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
        is_text = is_file_text(os.path.basename(file))
        var_ident = file.replace(".", "_").replace("-", "_")
        var_contents = bytes_to_cstr(src_data, is_text)
        lines += [f"A({var_ident}, {var_contents});\n"]
        file_list_content += [
            f'    F("{file}", {var_ident}, sizeof({var_ident}){"" if is_text else " - 1"}),\n'
        ]

    lines += ["\n", "const struct FWDescriptor firmware[] = {\n"]
    lines += file_list_content
    lines += ["};\n", "const size_t firmwareCount = arrsize(firmware);\n"]

    with open(target_file, "w") as file:
        file.writelines(lines)


if __name__ == "__main__":
    process_files(sys.argv[1], sys.argv[2])
