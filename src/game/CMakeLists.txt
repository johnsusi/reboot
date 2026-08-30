add_executable(game)

target_compile_features(core PRIVATE cxx_std_23)

target_sources(game
    PRIVATE
        main.cpp
)

target_link_libraries(game
    PRIVATE
        engine
)