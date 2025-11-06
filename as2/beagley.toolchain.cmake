# --- beagley.toolchain.cmake ---
#
# CMake toolchain file for cross-compiling for the BeagleY-AI.
#
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# --- YOU MUST EDIT THIS ---
# Set the prefix of your cross-compiler.
# Find this by running "ls /usr/bin | grep gcc"
# It's probably "aarch64-linux-gnu-" or "arm-linux-gnueabihf-"
set(COMPILER_PREFIX "aarch64-linux-gnu-")
# -------------------------

# Find the C and C++ compilers
set(CMAKE_C_COMPILER   ${COMPILER_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${COMPILER_PREFIX}g++)

# --- THIS IS THE CRITICAL FIX ---
# Set the path to the sysroot you just created
set(CMAKE_SYSROOT "$ENV{HOME}/beagley_sysroot")

# Configure CMake's "find" commands to look in the sysroot first
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)