cmake_minimum_required(VERSION 3.16)

project(PacketScope LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt options
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Force build type to Release if not set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Sanitizer options
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ASAN and TSAN cannot be enabled at the same time")
endif()

# Dependencies
set(CMAKE_PREFIX_PATH "/opt/Qt/6.10.1/gcc_64")
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Threads REQUIRED)
find_package(PcapPlusPlus REQUIRED)

# Sources
set(SOURCES
    src/main.cpp
    src/core/PacketCapture.cpp
    src/core/PacketProcessor.cpp
    src/core/PacketStore.cpp
    src/core/PipelineController.cpp
    src/ui/MainWindow.cpp
    src/ui/PacketListModel.cpp
)

# Headers which are includes Q_OBJECT
set(MOC_HEADERS
    include/ui/MainWindow.hpp
    include/ui/PacketListModel.hpp
)
qt_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})

add_executable(packet-scope
    ${SOURCES}
    ${MOC_SOURCES}
)

# Warning & optimization flags
set(COMMON_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wsign-conversion
    -Wimplicit-fallthrough
    -Wconversion
)

set(DEBUG_FLAGS -g -O0)
set(RELEASE_FLAGS -O3)
set(DEBUG_LINK_FLAGS "")

target_compile_options(packet-scope PRIVATE
    $<$<CONFIG:Debug>:${COMMON_WARNINGS} ${DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${RELEASE_FLAGS}>
)

# Sanitizers (append to debug flags)
if(ENABLE_ASAN)
    list(APPEND DEBUG_FLAGS -fsanitize=address)
    list(APPEND DEBUG_LINK_FLAGS -fsanitize=address)
endif()

if(ENABLE_TSAN)
    list(APPEND DEBUG_FLAGS -fsanitize=thread)
    list(APPEND DEBUG_LINK_FLAGS -fsanitize=thread)
endif()

# Apply flags
target_compile_options(packet-scope PRIVATE
    $<$<CONFIG:Debug>:${COMMON_WARNINGS} ${DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${RELEASE_FLAGS}>
)

if(DEBUG_LINK_FLAGS)
    target_link_options(packet-scope PRIVATE
        $<$<CONFIG:Debug>:${DEBUG_LINK_FLAGS}>
    )
endif()

# Include directories
target_include_directories(packet-scope
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(packet-scope
    PRIVATE
        Qt6::Widgets
        PcapPlusPlus::Pcap++
        Threads::Threads
)

# Build summary
message(STATUS "")
message(STATUS "========= PacketScope Build Configuration =========")
message(STATUS "Build type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard      : C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler          : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "ASAN enabled      : ${ENABLE_ASAN}")
message(STATUS "TSAN enabled      : ${ENABLE_TSAN}")

if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    message(STATUS "Debug flags       : ${COMMON_WARNINGS} ${DEBUG_FLAGS}")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    message(STATUS "Release flags     : ${RELEASE_FLAGS}")
endif()

message(STATUS "")