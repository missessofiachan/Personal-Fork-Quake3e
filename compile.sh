#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
# pipefail ensures pipeline failures are caught
set -eo pipefail

# Get the directory of this script safely
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR" || exit 1

# --- Configuration Constants ---
BUILD_DIR="build"
TARGET_CONTAINER="Bazzite-dev-env"
Q3_PATH="${SCRIPT_DIR}"

# Setup color codes safely (disabled if output is redirected or not a TTY)
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    BLUE='\033[0;34m'
    YELLOW='\033[0;33m'
    RED='\033[0;31m'
    NC='\033[0m' # No Color
else
    GREEN=''
    BLUE=''
    YELLOW=''
    RED=''
    NC=''
fi

# 1. Parse arguments
CLEAN_BUILD=false
RUN_AFTER_BUILD=false
RUN_ARGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -r|--run)
            RUN_AFTER_BUILD=true
            shift
            # Collect all remaining arguments to pass to the game binary
            while [[ $# -gt 0 ]]; do
                RUN_ARGS+=("$1")
                shift
            done
            ;;
        -h|--help)
            echo "Usage: $0 [options] [--run game-args...]"
            echo ""
            echo "Options:"
            echo "  -c, --clean     Perform a clean build by deleting the ${BUILD_DIR}/ directory first."
            echo "  -r, --run       Run quake3e automatically after a successful compilation."
            echo "                  All subsequent arguments are forwarded directly to the game."
            echo "  -h, --help      Display this help menu."
            echo ""
            echo "Examples:"
            echo "  $0 -c"
            echo "  $0 -r +set s_volume 0.5"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}" >&2
            echo "Use -h or --help for usage information." >&2
            exit 1
            ;;
    esac
done

echo -e "${BLUE}=== quake3e Build Script ===${NC}"

if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Clean build requested. Removing ${BUILD_DIR}/ directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# 2. Detect container environment vs host
INSIDE_CONTAINER=false
if [ -f /run/.containerenv ] || [ -f /run/.toolboxenv ] || [[ "${container:-}" =~ ^(podman|docker)$ ]]; then
    INSIDE_CONTAINER=true
fi

# Determine how to run commands (directly or via distrobox)
USE_DISTROBOX=false
if [ "$INSIDE_CONTAINER" = false ]; then
    if command -v distrobox &> /dev/null; then
        # Safely check if the target container exists
        if distrobox list 2>/dev/null | grep -q "$TARGET_CONTAINER"; then
            USE_DISTROBOX=true
        fi
    fi
fi

# Helper function to run commands in the correct context
run_build_cmd() {
    if [ "$USE_DISTROBOX" = true ]; then
        distrobox enter "$TARGET_CONTAINER" -- "$@"
    else
        "$@"
    fi
}

if [ "$USE_DISTROBOX" = true ]; then
    echo -e "${BLUE}Running build commands inside '$TARGET_CONTAINER' distrobox container...${NC}"
else
    if [ "$INSIDE_CONTAINER" = true ]; then
        echo -e "${BLUE}Running build commands directly inside container...${NC}"
    else
        echo -e "${YELLOW}Warning: Distrobox '$TARGET_CONTAINER' not found. Compiling on host...${NC}"
    fi
fi

# 3. Ensure required build dependencies are present
# These are the Fedora/RHEL package names needed to compile quake3e
REQUIRED_PKGS=(
    libcurl-devel
    SDL2-devel
    mesa-libGL-devel
    gcc
    make
)

check_and_install_deps() {
    local missing=()
    for pkg in "${REQUIRED_PKGS[@]}"; do
        if ! rpm -q "$pkg" &>/dev/null; then
            missing+=("$pkg")
        fi
    done
    if [ ${#missing[@]} -gt 0 ]; then
        echo -e "${YELLOW}Installing missing build dependencies: ${missing[*]}${NC}"
        sudo dnf install -y "${missing[@]}"
    else
        echo -e "${GREEN}All build dependencies are present.${NC}"
    fi
}

if [ "$USE_DISTROBOX" = true ]; then
    distrobox enter "$TARGET_CONTAINER" -- bash -c "
        missing=()
        for pkg in ${REQUIRED_PKGS[*]}; do
            rpm -q \"\$pkg\" &>/dev/null || missing+=(\"\$pkg\")
        done
        if [ \${#missing[@]} -gt 0 ]; then
            echo \"Installing missing build dependencies: \${missing[*]}\"
            sudo dnf install -y \"\${missing[@]}\"
        fi
    "
elif command -v rpm &>/dev/null && command -v dnf &>/dev/null; then
    check_and_install_deps
fi

# Determine parallel build job count
if [ -n "${JOBS:-}" ]; then
    NUM_JOBS="$JOBS"
elif command -v nproc &>/dev/null; then
    NUM_JOBS=$(nproc)
elif command -v sysctl &>/dev/null; then
    NUM_JOBS=$(sysctl -n hw.ncpu)
else
    NUM_JOBS=2
fi

# Start build timer
START_TIME=$(date +%s)

# 3. Build the project with make
echo -e "${GREEN}Compiling quake3e with ${NUM_JOBS} parallel jobs...${NC}"
if ! run_build_cmd make -j"${NUM_JOBS}"; then
    echo -e "${RED}Build failed! Try a clean build:${NC}" >&2
    echo -e "${YELLOW}  $0 --clean${NC}" >&2
    exit 1
fi

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

# Locate the compiled client binary (e.g. build/release-linux-x86_64/quake3e.x64)
# The Makefile uses: build/release-<platform>-<arch>/quake3e<archext>
# Exclude the dedicated server (quake3e.ded*) and renderer shared libs (*.so)
CLIENT_BIN=$(find "$BUILD_DIR" -type f -name "quake3e*" -executable \
    ! -name "quake3e.ded*" ! -name "*.so" -print -quit 2>/dev/null || true)

if [ -z "$CLIENT_BIN" ]; then
    echo -e "${RED}Build succeeded, but couldn't locate the quake3e client binary under ${BUILD_DIR}/!${NC}" >&2
    exit 1
fi

echo -e "${GREEN}Build complete in ${ELAPSED}s! Executable located at:${NC}"
echo -e "${BLUE}  ${CLIENT_BIN}${NC}"

# 4. Optional Auto-Run
if [ "$RUN_AFTER_BUILD" = true ]; then
    echo -e "${GREEN}Launching quake3e with arguments: ${RUN_ARGS[*]:-<none>}...${NC}"
    "./${CLIENT_BIN}" +set fs_basepath "${Q3_PATH}" "${RUN_ARGS[@]}"
fi
