#!/bin/bash

# Installs everything needed to generate and build MRBind bindings.
# NOTE: On old Ubuntu (20.04?) this will build a newer GNU Make from source and install it to `/usr/local/bin`.

# Must run this as root.

set -euxo pipefail

apt update
# Install `xargs` because we need it below.
apt install -y findutils

# Read the Clang version from `preferred_clang_version.txt`. `xargs` trims the whitespace.
SCRIPT_DIR="$(realpath "$(dirname "$BASH_SOURCE")")"
CLANG_VER="$(cat $SCRIPT_DIR/preferred_clang_version.txt | xargs)"
[[ $CLANG_VER ]] || (echo "Not sure what version of Clang to use." && false)

# Add LLVM repositories if the required package is not accessible right now.
# If it's accessible, either we have already added the same repos, or the version of Ubuntu is new enough to have it in the official repos.
# `-s` means dry run (check if it's installable or not).
if ! apt-get install -s clang-$CLANG_VER >/dev/null 2>/dev/null; then
    # This is what `llvm.sh` needs (search `apt install` in it and see for yourself).
    apt install -y lsb-release wget software-properties-common gnupg
    # Download `llvm.sh`.
    DIR="$(mktemp -d)"
    pushd $DIR
    wget https://apt.llvm.org/llvm.sh
    chmod +x llvm.sh
    ./llvm.sh "$CLANG_VER"
    popd
    rm -rf "$DIR"
fi

# Install the packages.
# Could also add `sudo` here for `install_mrbind_ubuntu.sh`, but I think the user can do that themselves.
apt install -y make cmake ninja-build gawk clang-$CLANG_VER lld-$CLANG_VER clang-tools-$CLANG_VER libclang-$CLANG_VER-dev llvm-$CLANG_VER-dev