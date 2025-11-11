cmake_minimum_required(VERSION 3.12)
project(gr2cpp VERSION 1.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library source files
set(GR2_SOURCES
    src/format.cpp
    src/model.cpp
    src/reader.cpp
)

set(GR2_HEADERS
    include/gr2/math_types.h
    include/gr2/format.h
    include/gr2/model.h
    include/gr2/reader.h
)

# Create library
add_library(gr2 STATIC ${GR2_SOURCES} ${GR2_HEADERS})

target_include_directories(gr2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Compiler warnings
if(MSVC)
    target_compile_options(gr2 PRIVATE /W4)
else()
    target_compile_options(gr2 PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Example executable
add_executable(gr2_example examples/example.cpp)
target_link_libraries(gr2_example PRIVATE gr2)

# Installation
install(TARGETS gr2
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# Print configuration
message(STATUS "GR2 C++ Library Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
