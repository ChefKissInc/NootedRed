#!/usr/bin/python3

# Copyright © 2022-2025 ChefKiss. Licensed under the Thou Shalt Not Profit License version 1.5.
# See LICENSE for details.

import os
import sys
import json

header = """#include <Headers/kern_util.hpp>
#include <PrivateHeaders/Firmware.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/SWIP/DMCU.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/SWIP/GC.hpp>
#include <PrivateHeaders/GPUDriversAMD/CAIL/SWIP/SDMA.hpp>

#define A(I, D) static const UInt8 I[] = D 
#define GC(I, V, F8, RS, APO, PSD, F18, F1C, R, C, F2C, F2E, PO) \\
    static const GCFirmwareConstant I = { \\
        .version = V, \\
        .field8 = F8, \\
        .romSize = RS, \\
        .actualPayloadOffDWords = APO, \\
        .payloadSizeDWords = PSD, \\
        .field18 = F18, \\
        .field1C = F1C, \\
        .rom = R, \\
        .checksum = C, \\
        .field2C = F2C, \\
        .field2E = F2E, \\
        .payloadOffDWords = PO, \\
    }
#define DMCU(I, LA, RS, R) static const DMCUFirmwareConstant I = {.loadAddress = LA, .romSize = RS, .rom = R}
#define SDMA(I, V, RS, R, F18, POD, C) \\
    static const SDMAFWConstant I = { \\
        .version = V, \\
        .romSize = RS, \\
        .rom = R, \\
        .field18 = F18, \\
        .payloadOffDWords = POD, \\
        .checksum = C, \\
    }
#define F(I, D, L, E) {.name = I, .metadata = {.data = D, .length = L, .extra = E}}
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


def byte_to_char(b: int, is_text: bool):
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


def bytes_to_cstr(data: bytes, is_text: bool = False):
    return '"' + "".join(byte_to_char(b, is_text) for b in data) + '"'


def is_file_excluded(name: str) -> bool:
    return name.startswith(".") or name == "LICENSE"


def is_file_text(name: str) -> bool:
    return not name.endswith(".dat") and not name.endswith(".bin")


def process_files(target_file: str, fw_defs_file: str, dir: str):
    with open(fw_defs_file, "r") as f:
        fw_defs: dict[str, dict[str, str]] = json.load(f)
    os.makedirs(os.path.dirname(target_file), exist_ok=True)
    lines = header.splitlines(keepends=True) + ["\n"]
    file_list_content: list[str] = []
    files = sorted(
        filter(
            lambda v: not is_file_excluded(os.path.basename(v[1])),
            [(root, file) for root, _, files in os.walk(dir) for file in files],
        ),
        key=lambda v: v[1],
    )
    for root, file in files:
        with open(os.path.join(root, file), "rb") as src_file:
            src_data = src_file.read()
        is_text = is_file_text(os.path.basename(file))
        src_data_len = hex(len(src_data) + (1 if is_text else 0))
        var_ident = file.replace(".", "_").replace("-", "_")
        fw_def = fw_defs.get(file, None)
        var_contents = bytes_to_cstr(src_data, is_text)
        if fw_def is None:
            fw_def_var = "nullptr"
        else:
            assert not is_text
            fw_def_var = "&" + var_ident
            var_ident = "_" + var_ident.upper()
        lines += [f"A({var_ident}, {var_contents});\n"]
        if file.startswith("gc_"):
            assert fw_def is not None
            version = fw_def["version"]
            field_8 = fw_def["field8"]
            actual_payload_off_dws = fw_def["actualPayloadOffDWords"]
            payload_size_dws = fw_def["payloadSizeDWords"]
            field_18 = fw_def["field18"]
            field_1c = fw_def["field1C"]
            checksum = fw_def["checksum"]
            field_2c = fw_def["field2C"]
            field_2e = fw_def["field2E"]
            payload_off_dws = fw_def["payloadOffDWords"]
            lines += [
                f'GC({fw_def_var[1::]}, "{version}", {field_8}, {src_data_len}, {actual_payload_off_dws}, {payload_size_dws}, {field_18}, {field_1c}, {var_ident}, {checksum}, {field_2c}, {field_2e}, {payload_off_dws});\n'
            ]
        elif file.startswith("dmcu_"):
            assert fw_def is not None
            load_address = fw_def["loadAddress"]
            lines += [
                f"DMCU({fw_def_var[1::]}, {load_address}, {src_data_len}, {var_ident});\n"
            ]
        elif file.startswith("sdma_"):
            assert fw_def is not None
            version = fw_def["version"]
            field_18 = fw_def["field18"]
            payload_off_dws = fw_def["payloadOffDWords"]
            checksum = fw_def["checksum"]
            lines += [
                f'SDMA({fw_def_var[1::]}, "{version}", {src_data_len}, {var_ident}, {field_18}, {payload_off_dws}, {checksum});\n'
            ]
        else:
            assert fw_def is None
        file_list_content += [
            f'    F("{file}", {var_ident}, {src_data_len}, {fw_def_var}),\n'
        ]

    lines += ["\n", "const struct FWDescriptor firmware[] = {\n"]
    lines += file_list_content
    lines += ["};\n", "const size_t firmwareCount = arrsize(firmware);\n"]

    with open(target_file, "w") as file:
        file.writelines(lines)


if __name__ == "__main__":
    process_files(sys.argv[1], sys.argv[2], sys.argv[3])
