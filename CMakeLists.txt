cmake_minimum_required(VERSION 4.4)

project(Game
    DESCRIPTION "A small game"
    LANGUAGES CXX
    VERSION 1.0.0
)

list(PREPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_LIST_DIR}/cmake"
)

include(CTest)
include(testing)

find_package(assimp CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(unofficial-lua CONFIG REQUIRED)

add_subdirectory(src/core)
add_subdirectory(src/engine)
add_subdirectory(src/game)


if (BUILD_TESTING)
    # add_subdirectory(tests)
endif()