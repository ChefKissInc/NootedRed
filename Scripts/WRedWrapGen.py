#!/usr/bin/python3


def fix_type(type: str) -> str:
    assert "const" not in type

    if type in ["uchar", "byte", "undefined1"]:
        ret = "uint8_t"
    elif type in ["short", "ushort", "undefined2"]:
        ret = "uint16_t"
    elif type in ["int", "uint", "undefined4", "AMDReturn"]:
        ret = "uint32_t"
    elif type in ["long", "ulong", "ulonglong", "undefined8"]:
        ret = "uint64_t"
    elif type != "char *" and "*" in type:
        ret = "void *"
    else:
        ret = type

    return ret


def parse_param(param: str) -> tuple[str, str]:
    if "*" in param:
        type, name = [v.strip() for v in param.split("*")]
        type += " *"
    elif " " in param:
        type, name = [v.strip() for v in param.split(" ")]
    else:
        raise AssertionError()

    return (
        fix_type(type),
        "that" if name == "this" else name.replace("param_", "param"),
    )


def get_fmt_name(name: str) -> str:
    return "that" if name == "this" else name


def get_fmt_type(type: str) -> str:
    table = {
        "uint64_t": "0x%llX",
        "uint32_t": "0x%X",
        "uint16_t": "0x%hX",
        "uint8_t": "0x%hhX",
        "bool": "%d",
        "char *": "%s",
        "char": "%c",
        "IOReturn": "0x%X",
    }

    if type not in table and type.endswith(" *"):
        return "%p"

    assert type in table
    return table[type]


def to_pascal_case(inp: str) -> str:
    ret = ""
    i = 0
    while i < len(inp):
        if inp[i] == "_":
            if i + 1 == len(inp):
                break
            ret += inp[i + 1].upper()
            i += 1
        else:
            ret += inp[i]
        i += 1
    return ret[0].upper() + ret[1:]


def locate_line(lines: list[str], needle: str) -> int:
    for i, line in enumerate(lines):
        if needle in line:
            return i
    raise AssertionError()


cpp_path: str = "./WhateverRed/kern_wred.cpp"
hpp_path: str = "./WhateverRed/kern_wred.hpp"

with open(cpp_path) as cpp_file:
    cpp_lines: list[str] = cpp_file.readlines()

with open(hpp_path) as hpp_file:
    hpp_lines: list[str] = hpp_file.readlines()

signature: str = input("Signature from \"Edit Function\": ").strip().replace(
    " *", "*")
signature_parts = signature.split(" ")

return_type = fix_type(signature_parts[0].replace("*", " *"))

func_ident = signature_parts[1]
func_ident_pascal = to_pascal_case(func_ident)

parameters = [x.strip()
              for x in signature.split("(")[1].split(")")[0].split(",")]
parameters = [parse_param(x) for x in parameters]

params_stringified = ", ".join([" ".join(x) for x in parameters])

target_line = len(cpp_lines)
function = [
    "\n",
    f"{return_type} WRed::wrap{func_ident_pascal}({params_stringified}) {{\n",
]

fmt_types = " ".join(
    f"{get_fmt_name(x[1])}: {get_fmt_type(x[0])}" for x in parameters)
arguments = ", ".join(x[1] for x in parameters)
function.append(
    f"    DBGLOG(\"wred\", \"{func_ident} << ({fmt_types})\", {arguments});\n")

if return_type == "void":
    function.append(
        f"    FunctionCast(wrap{func_ident_pascal}, callbackWRed->org{func_ident_pascal})({arguments});\n")
    function.append(f"    DBGLOG(\"wred\", \"{func_ident} >> void\");\n")
else:
    function.append(
        f"    auto ret = FunctionCast(wrap{func_ident_pascal}, callbackWRed->org{func_ident_pascal})({arguments});\n")
    function.append(
        f"    DBGLOG(\"wred\", \"{func_ident} >> {get_fmt_type(return_type)}\", ret);\n")
    function.append("    return ret;\n")

function.append("}\n")  # -- End of function --
function.append("\n")

cpp_lines[target_line:target_line] = function  # Extend at index

kext: str = input("Kext: ").strip()
symbol: str = input("Symbol: ").strip()
target_line: int = locate_line(
    cpp_lines, f"Failed to route {kext} symbols")

while not cpp_lines[target_line].endswith("};\n"):
    target_line -= 1

indent = cpp_lines[target_line][:-3]

cpp_lines.insert(
    target_line, f"{indent}    {{\"{symbol}\", wrap{func_ident_pascal}, org{func_ident_pascal}}},\n")

target_line = len(hpp_lines) - 1
while hpp_lines[target_line] != "};\n":
    target_line -= 1

hpp_lines[target_line:target_line] = [
    f"    mach_vm_address_t org{func_ident_pascal}{{}};\n",
    f"    static {return_type} wrap{func_ident_pascal}({params_stringified});\n",
]

with open(cpp_path, "w") as f:
    f.write("".join(cpp_lines))

with open(hpp_path, "w") as f:
    f.write("".join(hpp_lines))
