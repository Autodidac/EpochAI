#!/bin/bash
# Usage: ./install.sh [gcc|clang] [Debug|Release]

# Determine compiler and assign a friendly name for the install folder
if [ "$1" == "gcc" ]; then
  COMPILER_NAME="gcc"
elif [ "$1" == "clang" ]; then
  COMPILER_NAME="clang"
else
  echo "Usage: $0 [gcc|clang] [Debug|Release]"
  exit 1
fi

# Set build type (default to Debug if not provided)
if [ -z "$2" ]; then
  BUILD_TYPE="Debug"
else
  BUILD_TYPE="$2"
fi

# Build directory matching the naming convention from the build script
BUILD_DIR="build/${COMPILER_NAME}/${BUILD_TYPE}"

# Install prefix shared with the build configuration step
INSTALL_PREFIX="${PWD}/built"

# Perform the installation, forwarding the configuration for multi-config generators
cmake --install "$BUILD_DIR" --config "$BUILD_TYPE" --prefix "$INSTALL_PREFIX"
