cmake_minimum_required(VERSION 3.25)
project(CoreLock 
    VERSION 1.0.0
    LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(corelock 
    src/corelock.c
)

target_include_directories(corelock PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)
target_link_libraries(corelock PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(corelock PRIVATE /W4 /WX)
else()
    target_compile_options(corelock PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(CoreLock::corelock ALIAS corelock)

install(TARGETS corelock
    EXPORT CoreLockTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES include/corelock.h DESTINATION include)

install(EXPORT CoreLockTargets
    FILE CoreLockTargets.cmake
    NAMESPACE CoreLock::
    DESTINATION lib/cmake/CoreLock
)
