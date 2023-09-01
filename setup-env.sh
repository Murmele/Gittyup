#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "`dirname "$0"`"

PRE_COMMIT=${SCRIPT_DIR}/.venv/bin/pre-commit

pre_commit() {
	# Set up pre-commit hook
	if [ ! -f "${PRE_COMMIT}" ]; then
		pushd ${SCRIPT_DIR}
		python3 -m venv .venv
		.venv/bin/pip install pre-commit
		popd
	fi
	${PRE_COMMIT} install
}

remove_pre_commt() {
	if [ -f "${PRE_COMMIT}" ]; then
		${PRE_COMMIT} uninstall
	fi
}

if [ $# -eq 0 ]; then
	set -- --help
fi

for arg in "$@"; do
	case $arg in
	pre_commit | pre-commit | install_pre_commit |  install-pre-commit)
		pre_commit
	;;
	remove_pre_commit | remove-pre-commit | uninstall_pre_commit |  uninstall-pre-commit)
		remove_pre_commit
	;;
	*)
		echo "USAGE $0 [commands]"
		echo "  Commands:"
		echo "    install-pre-commit - installs pre-commit hooks"
		echo "    uninstall-pre-commit - removes pre-commit hooks"
	;;
	esac
done
