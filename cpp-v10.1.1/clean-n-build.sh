#!/usr/bin/env bash
set -euo pipefail

echo "====================="
echo "  Woflang CLEAN + BUILD"
echo "====================="

# Resolve the script’s real directory
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$SCRIPT_DIR"

echo "Project root: $PROJECT_ROOT"
cd "$PROJECT_ROOT"

echo ""
echo "Cleaning build system caches..."
echo "----------------------------------"

# Remove build dir fully & safely
rm -rf "$PROJECT_ROOT/build"

# Remove CMake cache + files
rm -rf "$PROJECT_ROOT/CMakeFiles"
rm -f  "$PROJECT_ROOT/CMakeCache.txt"

# Remove global CMake user cache
rm -rf ~/.cache/cmake ~/.cmake 2>/dev/null || true

# Remove Ninja stale state if present
find "$PROJECT_ROOT" -maxdepth 3 -type f -name ".ninja_deps" -delete 2>/dev/null || true
find "$PROJECT_ROOT" -maxdepth 3 -type f -name ".ninja_log"  -delete 2>/dev/null || true

echo ""
echo "Removing stray / corrupted / editor temp files..."
echo "----------------------------------"

find "$PROJECT_ROOT" -type f \
    \( -name "*~" -o -name "*.tmp" -o -name "*.swp" -o -name "*.swo" \
       -o -name "*.orig" -o -name "*.rej" -o -name "*.bak" \
       -o -name "#*#" -o -name ".*.swp" \) -delete

echo ""
echo "Configuring with CMake..."
echo "----------------------------------"

mkdir -p "$PROJECT_ROOT/build"
cd "$PROJECT_ROOT/build"

cmake -G Ninja ..

echo ""
echo "Building Woflang..."
echo "----------------------------------"

ninja -v

echo ""
echo "Build complete!"
echo "Run with:"
echo "  ./bin/woflang"
echo "  ./bin/woflang script.wof"
echo "  ./bin/woflang --help"
echo ""
