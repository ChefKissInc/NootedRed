#!/bin/sh

target_file="${PROJECT_DIR}/NootedRed/Firmware.cpp"
fw_defs="${PROJECT_DIR}/NootedRed/FirmwareDefs.json"
if [ -f "$target_file" ]; then
    rm -f "$target_file"
fi
while [ $# -gt 0 ];
do
    case $1 in
        -P) fw_files=$2
            shift
        ;;

    esac
    shift
done

script_file="${PROJECT_DIR}/Scripts/GenerateFirmware.py"
python3 "${script_file}" "${target_file}" "${fw_defs}" "${fw_files}"
