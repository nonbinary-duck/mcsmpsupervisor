cmake_minimum_required(VERSION 3.12)

# Set the project name
project(<PROJ>)

# Add warnings to GCC and clang
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Inlcude source files from the src dir
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
