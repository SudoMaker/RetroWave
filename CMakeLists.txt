cmake_minimum_required(VERSION 3.13.4)
project(RetroWave
LANGUAGES C
HOMEPAGE_URL https://github.com/SudoMaker/RetroWave
DESCRIPTION "A library to interface with RetroWave OPL3 (express) devices"
VERSION 0.1.0)

set(LIB_VERSION_MAJOR 0)
set(LIB_VERSION_MINOR 1)
set(LIB_VERSION_PATCH 0)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

set(CMAKE_CXX_STANDARD 17)
include(GNUInstallDirs)

#
# RetroWaveConfig.cmake File
#
include(CMakePackageConfigHelpers)
configure_package_config_file(
        cmake/RetroWaveConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/RetroWaveConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RetroWave"
)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/RetroWaveConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

if(${CMAKE_C_COMPILER} MATCHES "(C|c)lang|Xcode")
    message("-- Omg: you are using clang!")
    add_compile_options(-Wno-unknown-warning-option)
endif()

file(GLOB_RECURSE RETROWAVE_BOARD_SOURCES "RetroWaveLib/Board/*.c")
file(GLOB_RECURSE RETROWAVE_BOARD_HEADERS "RetroWaveLib/Board/*.h")
file(GLOB_RECURSE RETROWAVE_PLATFORM_SOURCES "RetroWaveLib/Platform/*.c")
file(GLOB_RECURSE RETROWAVE_PLATFORM_HEADERS "RetroWaveLib/Platform/*.h")
file(GLOB_RECURSE RETROWAVE_PROTOCOL_SOURCES "RetroWaveLib/Protocol/*.c")
file(GLOB_RECURSE RETROWAVE_PROTOCOL_HEADERS "RetroWaveLib/Protocol/*.h")

add_library(
        RetroWave

        RetroWaveLib/RetroWave.c RetroWaveLib/RetroWave.h

        ${RETROWAVE_BOARD_SOURCES}
        ${RETROWAVE_BOARD_HEADERS}
        ${RETROWAVE_PLATFORM_SOURCES}
        ${RETROWAVE_PLATFORM_HEADERS}
        ${RETROWAVE_PROTOCOL_SOURCES}
        ${RETROWAVE_PROTOCOL_HEADERS}
)

target_include_directories(RetroWave
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RetroWaveLib>
)

set_target_properties(RetroWave PROPERTIES
        VERSION ${LIB_VERSION_STRING} SOVERSION ${LIB_VERSION_MAJOR}
        C_VISIBILITY_PRESET hidden
)

install(FILES RetroWaveLib/RetroWave.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RetroWaveLib)
install(FILES ${RETROWAVE_BOARD_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RetroWaveLib/Board)
install(FILES ${RETROWAVE_PLATFORM_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RetroWaveLib/Platform)
install(FILES ${RETROWAVE_PROTOCOL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RetroWaveLib/Protocol)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(TARGETS RetroWave
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        EXPORT RetroWaveTargets
)

install(
        EXPORT RetroWaveTargets
        FILE RetroWaveTargets.cmake
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RetroWave"
        NAMESPACE RetroWave::
)

install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/RetroWaveConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/RetroWaveConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RetroWave"
)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/RetroWave.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/RetroWave.pc"
        @ONLY
)


set(RETROWAVE_BUILD_PLAYER -1 CACHE STRING "Set this to 0 to disable the player.")

if(${RETROWAVE_BUILD_PLAYER} EQUAL -1)
    message("-- Note: Building the player by default. This requires additional dependencies. Use -DRETROWAVE_BUILD_PLAYER=0 to disable.")
    set(RETROWAVE_BUILD_PLAYER 1)
endif()

if(${RETROWAVE_BUILD_PLAYER} EQUAL 1)
    set(CPM_DOWNLOAD_VERSION 0.27.5)
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

    if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
        message(STATUS "Downloading CPM.cmake")
        file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
    endif()

    include(${CPM_DOWNLOAD_LOCATION})

    CPMAddPackage(
            NAME cxxopts
            GITHUB_REPOSITORY jarro2783/cxxopts
            VERSION 3.0.0
            GIT_SHALLOW ON
    )

    CPMAddPackage(
            NAME TinyVGM
            GITHUB_REPOSITORY SudoMaker/TinyVGM
            VERSION 0.0.2
    )

    include_directories(${cxxopts_SOURCE_DIR}/include)

    add_executable(RetroWave_Player
            Player/Player.cpp Player/Player.hpp
            Player/SoundDriver.cpp Player/Controls.cpp Player/OSD.cpp Player/RegMap.cpp Player/Metadata.cpp)

    target_link_libraries(RetroWave_Player RetroWave TinyVGM z)
endif()

