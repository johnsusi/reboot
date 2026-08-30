add_library(core)

target_compile_features(core PUBLIC cxx_std_23)

target_sources(core
    PRIVATE
        assert.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS .. FILES
        assert.h
)