cmake_minimum_required(VERSION 3.15)
project(482_proj_1)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Platform-specific libthread object file
if(APPLE)
    set(LIBTHREAD_OBJ ${PROJECT_SOURCE_DIR}/lib/libthread_macos.o)
else()
    set(LIBTHREAD_OBJ ${PROJECT_SOURCE_DIR}/lib/libthread.o)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_BUILD_TYPE Debug)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Only include main.cpp to avoid duplicate symbols
add_executable(scheduler src/main.cpp ${LIBTHREAD_OBJ})
target_link_directories(scheduler PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(scheduler dl pthread)