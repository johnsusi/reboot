add_library(engine)

target_compile_features(engine PUBLIC cxx_std_23)

target_sources(engine
    PRIVATE
        asset.cpp
        input.cpp
        video.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS .. FILES
        asset.h
        input.h
        video.h
)

target_include_directories(engine PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(engine
    PRIVATE
        assimp::assimp
        Bullet3Dynamics
        Bullet3Collision
        Bullet3Common
        glad::glad
        nlohmann_json::nlohmann_json
        SDL3::SDL3
        unofficial::lua::lua-cpp
    PUBLIC
        glm::glm
)