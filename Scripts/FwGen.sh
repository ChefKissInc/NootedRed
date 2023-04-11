#!/bin/sh

script_file="${PROJECT_DIR}/Scripts/GenerateFirmware.py"
target_file="${PROJECT_DIR}/NootedRed/kern_fw.cpp"

if ! command -v python3 &> /dev/null; then
    echo "Python3 is not installed."
    exit 1
fi

if [ -z "${PROJECT_DIR}" ] || [ ! -d "${PROJECT_DIR}" ]; then
    PROJECT_DIR=$(pwd)
    script_file="${PROJECT_DIR}/Scripts/GenerateFirmware.py"
    target_file="${PROJECT_DIR}/NootedRed/kern_fw.cpp"
    if [ ! -f "${script_file}" ]; then
        echo "${script_file} is not exist."
    fi
fi
echo "Project directory is: ${PROJECT_DIR}"

if [ -f "$target_file" ]; then
    mv -f "$target_file"{,.old}
fi

fw_files=""
while getopts "P:" opt ; do
    case $opt in
        P)
            fw_files=${OPTARG}
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
    esac
done


python3 "${script_file}" "${target_file}" "${fw_files}"
