#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -eo pipefail

# Get the directory of this script safely
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR" || exit 1

# --- Configuration Constants ---
# BUILD_DIR matches the Makefile default
BUILD_DIR="build"
# Game files (baseq3/, missionpack/, etc.) live in the project root
Q3_PATH="${SCRIPT_DIR}"

# Setup color codes safely
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    NC='\033[0m'
else
    GREEN=''
    RED=''
    NC=''
fi

# 1. Locate the compiled client binary dynamically
# Makefile places it at: build/release-linux-<arch>/quake3e<archext>
# Exclude the dedicated server binary (quake3e.ded*)
CLIENT_BIN=$(find "$BUILD_DIR" -type f -name "quake3e*" -executable \
    ! -name "quake3e.ded*" ! -name "*.so" -print -quit 2>/dev/null || true)

if [ -z "$CLIENT_BIN" ]; then
    echo -e "${RED}Error: quake3e binary not found under ${BUILD_DIR}/!${NC}" >&2
    echo "Please run compile.sh first." >&2
    exit 1
fi

# 2. Verify the game assets directory exists
if [ ! -d "$Q3_PATH/baseq3" ]; then
    echo -e "${RED}Error: Quake 3 Arena game assets not found at:${NC}" >&2
    echo -e "  ${Q3_PATH}/baseq3" >&2
    exit 1
fi

echo -e "${GREEN}Launching quake3e...${NC}"

# 3. Execute with explicit basepath and forward all arguments ($@) cleanly
"./${CLIENT_BIN}" +set fs_basepath "$Q3_PATH" "$@"
