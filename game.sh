#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -eo pipefail

# Get the directory of this script safely
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR" || exit 1

# --- Configuration Constants ---
BUILD_DIR="build"
Q3_PATH="${SCRIPT_DIR}"

# Setup color codes safely
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    CYAN='\033[0;36m'
    RED='\033[0;31m'
    NC='\033[0m'
else
    GREEN=''
    CYAN=''
    RED=''
    NC=''
fi

# 1. Locate the compiled client binary dynamically
CLIENT_BIN=$(find "$BUILD_DIR" -type f \( -name "quake3e.x86_64" -o -name "quake3e.x64" -o -name "quake3e" \) -executable \
    ! -name "quake3e.ded*" ! -name "*.so" -print -quit 2>/dev/null || true)

if [ -z "$CLIENT_BIN" ]; then
    echo -e "${RED}Error: quake3e binary not found under ${BUILD_DIR}/!${NC}" >&2
    echo "Please run ./compile.sh first to build the client executable." >&2
    exit 1
fi

# 2. Verify the game assets directory exists
if [ ! -d "$Q3_PATH/baseq3" ]; then
    echo -e "${RED}Error: Quake 3 Arena game assets not found at:${NC}" >&2
    echo -e "  ${Q3_PATH}/baseq3" >&2
    exit 1
fi

echo -e "${CYAN}====================================================${NC}"
echo -e "${GREEN} Launching Quake3e with High-Performance Vulkan API... ${NC}"
echo -e "${CYAN}====================================================${NC}"

# 3. Execute with Vulkan renderer, explicit basepath, and pass through any additional arguments ($@)
exec "./${CLIENT_BIN}" +set cl_renderer vulkan +set fs_basepath "$Q3_PATH" "$@"
