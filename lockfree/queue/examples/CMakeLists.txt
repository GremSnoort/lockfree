set(project lockfree_queue_example)

PROJECT(${project} VERSION "${CMAKE_PROJECT_VERSION}" LANGUAGES CXX)

file(GLOB ${PROJECT_NAME}_INTERNAL_HEADERS
    ./*.h
    ./*.hpp
    ./../*.h
    ./../*.hpp
)

file(GLOB ${PROJECT_NAME}_INTERNAL_SOURCES
    ./lockfree_queue.cpp
)

set(${PROJECT_NAME}_HEADERS
    ${${PROJECT_NAME}_INTERNAL_HEADERS}
)

set(${PROJECT_NAME}_SOURCES
    ${${PROJECT_NAME}_INTERNAL_SOURCES}
)

message(STATUS "PROJECT_NAME = ${PROJECT_NAME}")

add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    cxxopts::cxxopts
    mimalloc-static
    sdk_benchmark_time
)
