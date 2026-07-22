cmake_minimum_required(VERSION 3.16)
project(F1SpiritRemake CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Dependencies
#
# All of these ship CMake config files in MSYS2 (mingw-w64-x86_64-SDL3,
# -SDL3_mixer, -SDL3_image, -curl) and on Linux distro packages / vcpkg, so
# plain find_package() is enough - no FetchContent needed.
# ---------------------------------------------------------------------------

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_mixer REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(CURL REQUIRED)
find_package(OpenGL REQUIRED)

# GLU has no standard CMake config; locate the raw library instead.
find_library(GLU_LIBRARY NAMES GLU glu32)
if(NOT GLU_LIBRARY)
    message(FATAL_ERROR "GLU library not found - install mingw-w64-x86_64-glu (MSYS2) or libglu1-mesa-dev (Debian/Ubuntu)")
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

file(GLOB F1SPIRIT_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/sources/*.cpp")

if(WIN32)
    add_executable(f1spirit WIN32 ${F1SPIRIT_SOURCES})
else()
    add_executable(f1spirit ${F1SPIRIT_SOURCES})
endif()

target_include_directories(f1spirit PRIVATE
    "${CMAKE_SOURCE_DIR}/sources"
    "${CMAKE_SOURCE_DIR}/sources/compat"
)

target_compile_options(f1spirit PRIVATE -Wno-write-strings)


target_link_libraries(f1spirit PRIVATE
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    SDL3_image::SDL3_image
    CURL::libcurl
    OpenGL::GL
    ${GLU_LIBRARY}
)

if(WIN32)
    # compat/SDL_net.h talks directly to Winsock.
    target_link_libraries(f1spirit PRIVATE ws2_32)
endif()

# ---------------------------------------------------------------------------
# Assets: the game loads "graphics/...", "sound/..." and "tracks/..." as
# paths relative to the working directory, plus f1spirit.cfg. Copy them
# next to the built executable so it runs straight out of the build tree.
# ---------------------------------------------------------------------------

foreach(asset_dir graphics sound tracks demos)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${asset_dir}")
        add_custom_command(TARGET f1spirit POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/${asset_dir}"
                    "$<TARGET_FILE_DIR:f1spirit>/${asset_dir}"
        )
    endif()
endforeach()

if(EXISTS "${CMAKE_SOURCE_DIR}/f1spirit.cfg")
    add_custom_command(TARGET f1spirit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/f1spirit.cfg"
                "$<TARGET_FILE_DIR:f1spirit>/f1spirit.cfg"
    )
endif()

# On Windows, MSYS2-built SDL3/SDL3_mixer/SDL3_image/curl DLLs need to sit
# next to the .exe (or on PATH) to run outside the MSYS2 shell.
if(WIN32)
    add_custom_command(TARGET f1spirit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t "$<TARGET_FILE_DIR:f1spirit>"
                $<TARGET_RUNTIME_DLLS:f1spirit>
        COMMAND_EXPAND_LISTS
    )
endif()
