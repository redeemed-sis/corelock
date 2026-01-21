cmake_minimum_required(VERSION 3.25)
project(CoreLockExamples LANGUAGES C)

add_library(example_utils utils.c)
target_link_libraries(example_utils PRIVATE CoreLock::corelock)

add_executable(dummy_rt dummy_rt.c)
target_link_libraries(dummy_rt PRIVATE CoreLock::corelock example_utils)
