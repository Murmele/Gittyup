#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "`dirname "$0"`"

# Variable that will hold the name of the clang-format command
FMT=""

FOLDERS=("./src" "./test" "./l10n")

# We specifically require clang-format v13. Some distros include the version
# number in the name, others don't. Prefer the specifically-named version.
for clangfmt in clang-format-13 clang-format
do
    if command -v "$clangfmt" &>/dev/null; then
        FMT="$clangfmt"
        break
    fi
done

# Check if we found a working clang-format
if [ -z "$FMT" ]; then
    echo "failed to find clang-format"
    exit 1
fi

# Check we have v13 of clang-format
VERSION=`$FMT --version | grep -Po 'version\s\K(\d+)'`
if [ "$VERSION" != "13" ]; then
	echo "Found clang-format v$VERSION, but v13 is required. Please install v13 of clang-format and try again."
	echo "On Debian-derived distributions, this can be done via: apt install clang-format-13"
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

# Format cmake files
# NOTE: requires support for python venv; on Debian-like distros, this can be
# installed using apt install python3-venv
echo "Start formatting cmake files"
CMAKE_FORMAT=${SCRIPT_DIR}/.venv/bin/cmake-format
if [ ! -f "$CMAKE_FORMAT" ]; then
	pushd ${SCRIPT_DIR}
	python3 -m venv .venv
	.venv/bin/pip install cmake-format==0.6.13
	popd
fi
find . \
    \( -type d -path './test/dep/*' -prune \) \
    -o \( -type d -path './dep/*/*' -prune \) \
    -o \( -name CMakeLists.txt -exec "$CMAKE_FORMAT" --in-place {} + \)
