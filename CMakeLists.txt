cmake_minimum_required(VERSION 3.23)
project(game_engine_jhinpan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")


# include directories
include_directories(
        ${CMAKE_SOURCE_DIR}/glm
        ${CMAKE_SOURCE_DIR}/rapidjson
        /opt/homebrew/include/SDL2
        ${CMAKE_SOURCE_DIR}/Lua
        ${CMAKE_SOURCE_DIR}/LuaBridge
        ${CMAKE_SOURCE_DIR}/box2d/include
        ${CMAKE_SOURCE_DIR}/box2d/include/box2d
        ${CMAKE_SOURCE_DIR}/emsdk/upstream/emscripten/system/include
)

# Set paths to SDL frameworks
set(SDL2_PATH "/Users/jhinpan/eecs498_gea/game_engine_jhinpan")
set(SDL2_IMAGE_PATH "/Users/jhinpan/eecs498_gea/game_engine_jhinpan")
set(SDL2_TTF_PATH "/Users/jhinpan/eecs498_gea/game_engine_jhinpan")
set(SDL2_MIXER_PATH "/Users/jhinpan/eecs498_gea/game_engine_jhinpan")

# Find SDL frameworks
find_library(SDL2 SDL2 PATHS ${SDL2_PATH})
find_library(SDL2_image SDL2_image PATHS ${SDL2_IMAGE_PATH})
find_library(SDL2_ttf SDL2_ttf PATHS ${SDL2_TTF_PATH})
find_library(SDL2_mixer SDL2_mixer PATHS ${SDL2_MIXER_PATH})

add_executable(game_engine_jhinpan main.cpp EngineUtils.h Actor.h Scene.h Helper.h AudioHelper.h Renderer.h IntroRunner.h AudioManager.h Input.h)

# Link SDL frameworks
target_link_libraries(game_engine_jhinpan ${SDL2} ${SDL2_image} ${SDL2_ttf} ${SDL2_mixer})
