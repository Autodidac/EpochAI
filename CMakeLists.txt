# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.20)

project(EpochAI VERSION 1.0 LANGUAGES CXX)

# Enforce the installation directory inside the repository so that local builds
# and the install script behave consistently across platforms.
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/built" CACHE PATH "Install path prefix" FORCE)

add_subdirectory("epochai")
















