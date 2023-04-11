#!/usr/bin/python3
import os
import struct
import sys
from pathlib import Path

header = '''
//  NootedRed
//
//  Copyright © 2023 ChefKiss Inc. All rights reserved.

#include "kern_fw.hpp"
'''

def format_file_name(file_name):
    return file_name.replace(".", "_").replace("-", "_")

def write_single_file(target_file, path, file):
    with open(path, "rb") as src_file:
        src_data = src_file.read()
        src_len = len(src_data)

    fw_var_name = format_file_name(file)
    target_str = "\nconst unsigned char " + fw_var_name + "[] = {"
    index = 0
    block = []
    while True:
        if index + 16 >= src_len:
            block = src_data[index:]
        else:
            block = src_data[index:index + 16]
        index += 16
        if len(block) < 16:
            if len(block):
                for b in block:
                    if type(b) is str:
                        b = ord(b)
                    target_str += "0x{:02X}, ".format(b)
                target_str += "\n"
            break
        target_str += ("0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}, "
                          "0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}, "
                          "0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X}, "
                          "0x{:02X}, 0x{:02X}, 0x{:02X}, 0x{:02X},\n"
                          .format(*struct.unpack("BBBBBBBBBBBBBBBB", block)))
    target_str += "};\n"
    target_str += "const long int " + fw_var_name + "_size = sizeof(" + fw_var_name + ");\n"
    target_file.write(target_str)


def process_files(target_file, dir):
    fw_desc_list = []

    if not os.path.exists(target_file):
        if not os.path.exists(os.path.dirname(target_file)):
            os.makedirs(os.path.dirname(target_file))

    with open(target_file, "w") as target_file_handle:
        target_file_handle.write(header)
        for path in Path(dir).glob("**/*"):
            if path.is_file():
                file = path.name
                write_single_file(target_file_handle, str(path), file)
                fw_var_name = format_file_name(file)
                fw_desc_list.append('{NRED_FW("' + file + '", ' + fw_var_name + ", " + fw_var_name + "_size)}")

        target_str = "\n"
        target_str += "const struct FwDesc fwList[] = {" + ",\n".join(fw_desc_list) + "};\n"
        target_str += "const int fwNumber = " + str(len(fw_desc_list)) + ";\n"
        target_file_handle.write(target_str)

if __name__ == '__main__':
    if not os.path.exists(sys.argv[2]): 
        exit(f"{sys.argv[2]} is not exist.")
    process_files(sys.argv[1], sys.argv[2])
