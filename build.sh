#!/bin/bash
# Usage: ./build.sh [gcc|clang] [Debug|Release]

# Set the installation prefix (shared with install.sh)
INSTALL_PREFIX="${PWD}/built"

# Select compiler and assign a friendly name for the build folder
if [ "$1" == "gcc" ]; then
  COMPILER_CXX="g++"
  COMPILER_NAME="gcc"
elif [ "$1" == "clang" ]; then
  COMPILER_CXX="clang++"
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

# Define the build directory including both compiler and build type
BUILD_DIR="build/${COMPILER_NAME}/${BUILD_TYPE}"

# Configure the project
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_CXX_COMPILER="$COMPILER_CXX" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"

# Build the project (verbose output for easier debugging)
cmake --build "$BUILD_DIR" --verbose
