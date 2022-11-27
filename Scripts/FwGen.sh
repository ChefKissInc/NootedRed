#!/bin/sh

target_file="${PROJECT_DIR}/LegacyRed/kern_fw.cpp"
if [ -f "$target_file" ]; then
rm -f $target_file
fi
while [ $# -gt 0 ];
do
    case $1 in
    -P) fw_files="$PROJECT_DIR/LegacyRed/Firmware"
    shift
    ;;
    
    esac
    shift
done

script_file="${PROJECT_DIR}/Scripts/GenerateFirmware.py"
python3 "${script_file}" "${target_file}" "${fw_files}"
