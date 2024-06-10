set(project lockfree_queue_benchmark)

PROJECT(${project} VERSION "${CMAKE_PROJECT_VERSION}" LANGUAGES CXX)

set(${PROJECT_NAME}_HEADERS
    #
)

set(${PROJECT_NAME}_SOURCES
    main.cpp
)

message(STATUS "PROJECT_NAME = ${PROJECT_NAME}")

add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    benchmark::benchmark_main
    mimalloc-static
    boost::boost
)