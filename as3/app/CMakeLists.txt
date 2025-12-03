# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(beatbox ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(beatbox LINK_PRIVATE hal asound gpiod)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET beatbox POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:beatbox>"
     "$ENV{HOME}/ensc351/public/myApps/beatbox" 
  COMMENT "Copying ARM executable to public NFS directory")

# Copy the 'beatbox-server' folder to ~/ensc351/public/myApps/beatbox-server-copy
add_custom_command(TARGET beatbox POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
     "${CMAKE_SOURCE_DIR}/beatbox-server-copy"
     "$ENV{HOME}/ensc351/public/myApps/beatbox-server-copy"
  COMMENT "Copying Node.js server to public NFS directory")

# Copy the 'wave-files' folder to ~/ensc351/public/myApps/beatbox-wave-files
add_custom_command(TARGET beatbox POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
     "${CMAKE_SOURCE_DIR}/beatbox-wav-files"
     "$ENV{HOME}/ensc351/public/myApps/beatbox-wav-files"
  COMMENT "Copying Wave files to public NFS directory")
