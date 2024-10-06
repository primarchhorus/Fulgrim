cmake_minimum_required(VERSION 3.5.0)
project(AudioPlayer VERSION 0.1.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(SndFile)

# Source files
set(SOURCES
    src/main.cpp
    src/file_io.cpp
    src/engine.cpp
)

# Header files
set(HEADERS
    src/circular_buffer.hpp
    src/file_io.h
    src/memory_allocators.hpp
    src/engine.h
)

# Create executable
add_executable(AudioPlayer ${SOURCES} ${HEADERS})
set_property(TARGET AudioPlayer PROPERTY CXX_STANDARD 17)
# find_library(AUDIO_UNIT AudioUnit)
# if (NOT AUDIO_UNIT)
#     message(FATAL_ERROR "AudioUnit not found")
# endif()
# target_link_libraries(AudioPlayer PRIVATE ${AUDIO_UNIT})
target_link_libraries(AudioPlayer PRIVATE SndFile::sndfile)
target_include_directories(AudioPlayer PRIVATE "thirdparty")

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
