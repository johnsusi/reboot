cmake_minimum_required(VERSION 4.4)

project(Game
    DESCRIPTION "A small game"
    LANGUAGES CXX
    VERSION 1.0.0
)

find_package(assimp CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(unofficial-lua)

add_subdirectory(src/core)
add_subdirectory(src/engine)
add_subdirectory(src/game)