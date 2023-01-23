#!/bin/bash

cd "`dirname "$0"`"

# Variable that will hold the name of the clang-format command
FMT=""

FOLDERS=("./src" "./test")

# Some distros just call it clang-format. Others (e.g. Ubuntu) are insistent
# that the version number be part of the command. We prefer clang-format if
# that's present, otherwise we work backwards from highest version to lowest
# version but at least 13.
for clangfmt in clang-format{,-{1,2,3}{9,8,7,6,5,4,3}}; do
    if which "$clangfmt" &>/dev/null; then
        FMT="$clangfmt"
        break
    fi
done

# Check if we found a working clang-format
if [ -z "$FMT" ]; then
    echo "failed to find clang-format"
    exit 1
fi

function format() {
    for f in $(find $@ \( -type d -path './test/dep/*' -prune \) -o \( -name '*.h' -or -name '*.m' -or -name '*.mm' -or -name '*.c' -or -name '*.cpp' \)); do
        echo "format ${f}";
        ${FMT} -i ${f};
    done

    echo "~~~ $@ Done ~~~";
}

for dir in ${FOLDERS[@]}; do
    if [ ! -d "${dir}" ]; then
        echo "${dir} is not a directory";
    else
        format ${dir};
    fi
done

echo "Start formatting cmake files"
pip install cmake-format==0.6.13
find . \
    \( -type d -path './test/dep/*' -prune \) \
    -o \( -type d -path './dep/*/*' -prune \) \
    -o \( -name CMakeLists.txt -exec cmake-format --in-place {} + \)
