project(App)

add_executable(App
    src/Utils/Log.cpp
    src/Main.cpp
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res
     DESTINATION ${CMAKE_BINARY_DIR}/App)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(App PUBLIC
        DEBUG
        LOGGING_ENABLED
        ASSERTS_ENABLED
    )
else()
    target_compile_definitions(App PUBLIC
        NDEBUG
    )
endif()

target_include_directories(App PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(App PRIVATE
    spdlog::spdlog_header_only
    tinygltf
)
