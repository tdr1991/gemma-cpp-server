cmake_minimum_required(VERSION 3.11) # or whatever minimum version you require

include(FetchContent)

project(gemma_cpp_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(sentencepiece GIT_REPOSITORY https://github.com/google/sentencepiece GIT_TAG 53de76561cfc149d3c01037f0595669ad32a5e7c)
FetchContent_MakeAvailable(sentencepiece)

FetchContent_Declare(gemma GIT_REPOSITORY https://github.com/google/gemma.cpp GIT_TAG origin/main)
FetchContent_MakeAvailable(gemma)

FetchContent_Declare(highway GIT_REPOSITORY https://github.com/google/highway.git GIT_TAG da250571a45826b21eebbddc1e50d0c1137dee5f)
FetchContent_MakeAvailable(highway)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Allowable types for WEIGHT_TYPE:
# float - slow, not recommended
# hwy::bfloat16_t - bfloat16 as implemented by https://github.com/google/highway
# SfpStream - 8-bit switched floating point (recommended)
# NuqStream - experimental, work-in-progress
option(WEIGHT_TYPE "Set weight type" "")

if (WEIGHT_TYPE)
  add_definitions(-DGEMMA_WEIGHT_T=${WEIGHT_TYPE})
endif()

# Executable Target
add_executable(server server.cpp)

target_link_libraries(server PRIVATE libgemma hwy hwy_contrib sentencepiece)

# Link against libgemma.a and any other necessary libraries
FetchContent_GetProperties(gemma)
FetchContent_GetProperties(sentencepiece)
target_include_directories(server PRIVATE ${gemma_SOURCE_DIR})
target_include_directories(server PRIVATE ${sentencepiece_SOURCE_DIR})
