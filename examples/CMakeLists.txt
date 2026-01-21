cmake_minimum_required(VERSION 3.25)
project(CoreLockExamples LANGUAGES C)

add_executable(dummy_rt dummy_rt.c)
target_link_libraries(dummy_rt PRIVATE CoreLock::corelock)
