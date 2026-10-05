cmake_minimum_required(VERSION 3.24)
project(NovaBrowserKernel LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Build/verify a replacement SDK without configuring either game branch or
# changing the normal Client build's pinned runtime before validation succeeds.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../../src/chrome" chrome)
