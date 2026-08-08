cmake_minimum_required(VERSION 3.20)
project(Nova2D VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "")
message(STATUS "  Nova2D Engine v${PROJECT_VERSION}")
message(STATUS "")

# ─── Platform Detection ──────────────────────────────────────────
if(WIN32)
    add_compile_definitions(NOVA_PLATFORM_WINDOWS)
elseif(ANDROID)
    add_compile_definitions(NOVA_PLATFORM_ANDROID)
elseif(APPLE)
    add_compile_definitions(NOVA_PLATFORM_APPLE)
elseif(UNIX)
    add_compile_definitions(NOVA_PLATFORM_LINUX)
endif()

# ─── SDL2 (pre-downloaded) ──────────────────────────────────────
set(SDL2_DIR "${CMAKE_SOURCE_DIR}/third_party/SDL2-2.30.2/x86_64-w64-mingw32")
set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include/SDL2")
set(SDL2_LIB_DIR "${SDL2_DIR}/lib")
set(SDL2_BIN_DIR "${SDL2_DIR}/bin")

# ─── stb (header-only, download if not present) ─────────────────
set(STB_DIR "${CMAKE_SOURCE_DIR}/third_party/stb")
if(NOT EXISTS "${STB_DIR}/stb_image.h")
    message(STATUS "Downloading stb_image.h...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
        "${STB_DIR}/stb_image.h"
        SHOW_PROGRESS
    )
endif()

# ─── nlohmann/json (header-only, download if not present) ───────
set(JSON_DIR "${CMAKE_SOURCE_DIR}/third_party/nlohmann")
if(NOT EXISTS "${JSON_DIR}/json.hpp")
    message(STATUS "Downloading nlohmann/json.hpp...")
    file(DOWNLOAD
        "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
        "${JSON_DIR}/json.hpp"
        SHOW_PROGRESS
    )
endif()

# ─── Source Files ─────────────────────────────────────────────────
file(GLOB_RECURSE NOVA_SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE NOVA_HEADERS "src/*.h")

# ─── Engine Executable ───────────────────────────────────────────
add_executable(Nova2D ${NOVA_SOURCES} ${NOVA_HEADERS})

target_include_directories(Nova2D PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIR}
    ${STB_DIR}
    ${JSON_DIR}
)

# Link SDL2
target_link_libraries(Nova2D PRIVATE
    -L${SDL2_LIB_DIR}
    -lmingw32 -lSDL2main -lSDL2 -mwindows
    -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32
    -lwinmm -limm32 -lole32 -loleaut32 -lshell32
    -lsetupapi -lversion -luuid -lhid -lopengl32
)

# ─── Copy SDL2 DLL to output ─────────────────────────────────────
add_custom_command(TARGET Nova2D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL2_BIN_DIR}/SDL2.dll"
    "$<TARGET_FILE_DIR:Nova2D>"
    COMMENT "Copying SDL2.dll..."
)

# ─── Copy Assets ─────────────────────────────────────────────────
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET Nova2D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Nova2D>/assets"
    )
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/scripts")
    add_custom_command(TARGET Nova2D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/scripts"
        "$<TARGET_FILE_DIR:Nova2D>/scripts"
    )
endif()

# ─── Compiler Options ────────────────────────────────────────────
if(MSVC)
    target_compile_options(Nova2D PRIVATE /W3)
else()
    target_compile_options(Nova2D PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

message(STATUS "Nova2D configuration complete!")
