#!/usr/bin/python3


def fixType(typ):
    if "const" in typ:
        raise AssertionError("Can't handle const types lol")

    if typ.startswith("uint") and "_t" in typ:
        return typ

    if typ == "bool":
        ret = "bool"
    elif typ in ["char", "uchar", "char*", "uchar*"]:
        ret = "uint8_t"
    elif typ in ["short", "ushort", "short*", "ushort*"]:
        ret = "uint16_t"
    elif typ in ["int", "uint", "int*", "uint*"]:
        ret = "uint32_t"
    elif typ in ["long", "ulong", "long*", "ulong*"]:
        ret = "uint64_t"
    elif typ == "void":
        ret = "void"
    else:
        if "*" in typ:
            return "void*"
        raise AssertionError("Failed parsing type: " + typ)

    if "*" in typ:
        ret += "*"

    return ret


def parseParam(param):
    if " " in param:
        typ, name = param.split(" ")
    elif "*" in param:
        typ, name = param.split("*")
        typ += "*"
    else:
        raise AssertionError("Failed parsing param: " + param)

    typ = fixType(typ)

    if name == "this":
        name = "that"

    name = name.replace("param_", "param")

    return (typ, name)


def getFormatName(name):
    if name == "that":
        name = "this"
    return name


def getFormatType(typ):
    if "*" in typ:
        return "%p"

    table = {"uint64_t": "0x%llX",
             "uint32_t": "0x%X",
             "uint16_t": "0x%hX",
             "uint8_t": "0x%hhX",
             "bool": "%d"}

    if typ in table:
        return table[typ]
    else:
        raise AssertionError("Failed getting format for: " + typ)


def toSnakeCase(inp):
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


def findLine(lines, needle):
    for i in range(len(lines)):
        if needle in lines[i]:
            return i
    raise AssertionError("Failed to locate line!")


cppPath = "./WhateverRed/kern_rad.cpp"
hppPath = "./WhateverRed/kern_rad.hpp"

cpp = open(cppPath).read().split("\n")
hpp = open(hppPath).read().split("\n")

sign = input("Copy full signature from the Edit function panel: ").strip().replace(" *", "*")
signParts = sign.split(" ")

retType = fixType(signParts[0])

funcName = signParts[1]
snakeName = toSnakeCase(funcName)

params = [x.strip() for x in sign.split("(")[1].split(")")[0].split(",")]
params = [parseParam(x) for x in params]

strParams = ", ".join([" ".join(x) for x in params])

injectLine = findLine(cpp, "bool RAD::processKext(KernelPatcher &patcher, size_t index,")
inject = []

if cpp[injectLine - 1] != "":
    inject.append("")

inject.append(f"{retType} RAD::wrap{snakeName}({strParams}) {{")

formatTypes = " ".join(f"{getFormatName(x[1])} = {getFormatType(x[0])}" for x in params)
args = ", ".join(x[1] for x in params)
inject.append(f"    NETLOG(\"rad\", \"{funcName}: {formatTypes}\", {args});")

if retType == "void":
    inject.append(f"    FunctionCast(wrap{snakeName}, callbackRAD->org{snakeName})({args});")
    inject.append(f"    NETLOG(\"rad\", \"{funcName} finished\");")
else:
    inject.append(f"    auto ret = FunctionCast(wrap{snakeName}, callbackRAD->org{snakeName})({args});")
    inject.append(f"    NETLOG(\"rad\", \"{funcName} returned {getFormatType(retType)}\", ret);")
if retType != "void":
    inject.append("    return ret;")
inject.append("}")
inject.append("")

# https://stackoverflow.com/a/7376026
# Magik
cpp[injectLine:injectLine] = inject


kextName = input("Input the kext name: ").strip()
symbolName = input("Input the symbol name: ").strip()
injectLine = findLine(cpp, f"Failed to route {kextName} symbols")

while not cpp[injectLine].endswith("};"):
    injectLine -= 1
indent = cpp[injectLine][:-2]

cpp.insert(injectLine, f"{indent}    {{\"{symbolName}\", wrap{snakeName}, org{snakeName}}},")


injectLine = len(hpp) - 1
while hpp[injectLine] != "};":
    injectLine -= 1

inject = []
if hpp[injectLine - 1] != "":
    inject.append("")
inject.append(f"    mach_vm_address_t org{snakeName}{{}};")
inject.append(f"    static {retType} wrap{snakeName}({strParams});")

hpp[injectLine:injectLine] = inject


with open(cppPath, "w") as f:
    f.write("\n".join(cpp))

with open(hppPath, "w") as f:
    f.write("\n".join(hpp))
