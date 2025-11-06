# CMakeList.txt for HAL
include_directories(hal/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})

target_include_directories(hal PUBLIC include)

# --- FIX: Find and link libgpiod ---
# This will now search inside your CMAKE_SYSROOT
find_library(GPIOD_LIBRARY gpiod)
if(NOT GPIOD_LIBRARY)
    message(FATAL_ERROR "Could not find libgpiod in sysroot!")
endif()

# Link the libraries
target_link_libraries(hal PUBLIC ${GPIOD_LIBRARY} pthread)