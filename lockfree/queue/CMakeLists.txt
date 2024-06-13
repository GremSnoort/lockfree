set(project lockfree_queue)

PROJECT(${project} VERSION "${CMAKE_PROJECT_VERSION}" LANGUAGES CXX)

set(${PROJECT_NAME}_HEADERS
    #
)

set(${PROJECT_NAME}_SOURCES
    source.cpp
)

message(STATUS "PROJECT_NAME = ${PROJECT_NAME}")

add_library(${PROJECT_NAME} SHARED
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    #
)

add_subdirectory(examples)
add_subdirectory(benchmarks)
add_subdirectory(tests)
