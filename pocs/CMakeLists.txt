# dependencies

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    find_package(Threads REQUIRED)
endif()

# third-party

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (EMSCRIPTEN)
else()
    if (NOT GGML_BACKEND_DL)
        add_subdirectory(vdot)
    endif()
endif()
