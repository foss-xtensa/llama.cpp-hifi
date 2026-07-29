set(TARGET llama-simple)
add_executable(${TARGET} simple.cpp)
install(TARGETS ${TARGET} RUNTIME)
target_link_libraries(${TARGET} PRIVATE llama ${CMAKE_THREAD_LIBS_INIT})
# Use set_property instead of target_compile_features — avoids compiler
# feature-detection failure on Xtensa cross-compiler (unknown compiler ID)
set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

# Xtensa HiFi5s: simple.cpp gets -O2 -Os to match Makefile SIMPLE_CXXFLAGS
# (NOT global -O3 which is used for ggml compute kernels)
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "xtensa")
    set_source_files_properties(simple.cpp PROPERTIES
        COMPILE_FLAGS "-O2 -Os -fno-strict-aliasing -fno-vectorize -DHIFI5_OPT"
    )
endif()
