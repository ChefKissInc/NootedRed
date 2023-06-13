#!/usr/bin/python3


def fix_type(type: str) -> str:
    assert "const" not in type

    if type in ["uchar", "byte", "undefined1"]:
        ret = "uint8_t"
    elif type in ["short", "ushort", "undefined2"]:
        ret = "uint16_t"
    elif type in ["int", "uint", "undefined4"]:
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
        "CAILResult": "0x%X",
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
    assert False


moduleToClass = {
    "hwlibs": "X5000HWLibs",
    "x5000": "X5000",
    "x6000": "X6000",
    "x6000fb": "X6000FB",
}

module: str = input("Filename without extension: ./NootedRed/kern_")
assert module in moduleToClass
className = moduleToClass[module]
cpp_path: str = f"./NootedRed/kern_{module}.cpp"
hpp_path: str = f"./NootedRed/kern_{module}.hpp"

with open(cpp_path) as cpp_file:
    cpp_lines: list[str] = cpp_file.readlines()

with open(hpp_path) as hpp_file:
    hpp_lines: list[str] = hpp_file.readlines()

signature: str = input("Signature from \"Edit Function\": ").strip().replace(
    " *", "*")
signature_parts: list[str] = signature.split(" ")

return_type: str = fix_type(signature_parts[0].replace("*", " *"))

func_ident: str = signature_parts[1]
func_ident_pascal: str = to_pascal_case(func_ident)

parameters: list[tuple[str, str]] = [parse_param(x) for x in [x.strip()
                                                              for x in signature.split("(")[1].split(")")[0].split(",")]]

params_stringified: str = ", ".join([" ".join(x) for x in parameters])

target_line: int = len(cpp_lines)
function: list[str] = [
    "\n",
    f"{return_type} {className}::wrap{func_ident_pascal}({params_stringified}) {{\n",
]

fmt_types: str = " ".join(
    f"{get_fmt_name(x[1])}: {get_fmt_type(x[0])}" for x in parameters)
arguments: str = ", ".join(x[1] for x in parameters)
function += [f"    DBGLOG(\"{module}\", \"{func_ident} << ({fmt_types})\", {arguments});\n"]

if return_type == "void":
    function += [
        f"    FunctionCast(wrap{func_ident_pascal}, callback->org{func_ident_pascal})({arguments});\n",
        f"    DBGLOG(\"{module}\", \"{func_ident} >> void\");\n",
    ]
else:
    function += [
        f"    auto ret = FunctionCast(wrap{func_ident_pascal}, callback->org{func_ident_pascal})({arguments});\n",
        f"    DBGLOG(\"{module}\", \"{func_ident} >> {get_fmt_type(return_type)}\", ret);\n",
        "    return ret;\n",
    ]

function += ["}\n"]  # -- End of function --

cpp_lines[target_line:target_line] = function  # Extend at index

symbol: str = input("Symbol: ").strip()
target_line: int = locate_line(
    cpp_lines, "Failed to route symbols")

while "};" not in cpp_lines[target_line]:
    target_line -= 1

indent = cpp_lines[target_line].split("};")[0]

cpp_lines.insert(
    target_line, f"{indent}    {{\"{symbol}\", wrap{func_ident_pascal}, this->org{func_ident_pascal}}},\n")

target_line = len(hpp_lines) - 1
while "wrap" not in hpp_lines[target_line]:
    target_line -= 1
while ");" not in hpp_lines[target_line]:
    target_line += 1
target_line += 1
hpp_lines[target_line:target_line] = [
    f"    static {return_type} wrap{func_ident_pascal}({params_stringified});\n",
]

while not hpp_lines[target_line].startswith("    mach_vm_address_t org"):
    target_line -= 1
target_line += 1
hpp_lines[target_line:target_line] = [
    f"    mach_vm_address_t org{func_ident_pascal} {{0}};\n",
]

with open(cpp_path, "w") as f:
    f.write("".join(cpp_lines))

with open(hpp_path, "w") as f:
    f.write("".join(hpp_lines))
