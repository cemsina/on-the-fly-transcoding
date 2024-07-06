cmake_minimum_required(VERSION 3.5)
project(otft CXX)

include(CheckIncludeFileCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lcurl")
check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
if (NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
    # Do nothing
elseif (HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
    set(CMAKE_CXX_STANDARD 20)
elseif (HAS_ANY AND HAS_STRING_VIEW)
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 14)
endif ()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include FFmpeg directories
include_directories(/usr/local/include)

# Link Directories
link_directories(/usr/local/lib)

aux_source_directory(src/common COMMON_SRC)
aux_source_directory(src/controller CONTROLLER_SRC)
aux_source_directory(src/net NET_SRC)
aux_source_directory(src/processor PROCESSOR_SRC)
aux_source_directory(src/utils UTILS_SRC)

add_executable(otft src/main.cpp)
target_sources(otft PRIVATE ${COMMON_SRC} ${CONTROLLER_SRC} ${NET_SRC} ${PROCESSOR_SRC} ${UTILS_SRC})

# Link FFmpeg libraries
target_link_libraries(otft
    avcodec
    avformat
    avutil
    swscale
    pthread
    curl
)
