set(project lockfree_queue_spsc_test)

PROJECT(${project} VERSION "${CMAKE_PROJECT_VERSION}" LANGUAGES CXX)

file(GLOB ${PROJECT_NAME}_INTERNAL_HEADERS
    ./*.h
    ./*.hpp
    ./data/*.h
    ./data/*.hpp
    ./../*.h
    ./../*.hpp
)

file(GLOB ${PROJECT_NAME}_INTERNAL_SOURCES
    ./spsc.cpp
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
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})

###
set(project lockfree_queue_mpsc_test)

PROJECT(${project} VERSION "${CMAKE_PROJECT_VERSION}" LANGUAGES CXX)

file(GLOB ${PROJECT_NAME}_INTERNAL_HEADERS
    ./*.h
    ./*.hpp
    ./data/*.h
    ./data/*.hpp
    ./../*.h
    ./../*.hpp
)

file(GLOB ${PROJECT_NAME}_INTERNAL_SOURCES
    ./mpsc.cpp
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
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})

###
set(project lockfree_queue_spmc_test)

PROJECT(${project} VERSION "${CMAKE_PROJECT_VERSION}" LANGUAGES CXX)

file(GLOB ${PROJECT_NAME}_INTERNAL_HEADERS
    ./*.h
    ./*.hpp
    ./data/*.h
    ./data/*.hpp
    ./../*.h
    ./../*.hpp
)

file(GLOB ${PROJECT_NAME}_INTERNAL_SOURCES
    ./spmc.cpp
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
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})

###
set(project lockfree_queue_mpmc_test)

PROJECT(${project} VERSION "${CMAKE_PROJECT_VERSION}" LANGUAGES CXX)

file(GLOB ${PROJECT_NAME}_INTERNAL_HEADERS
    ./*.h
    ./*.hpp
    ./data/*.h
    ./data/*.hpp
    ./../*.h
    ./../*.hpp
)

file(GLOB ${PROJECT_NAME}_INTERNAL_SOURCES
    ./mpmc.cpp
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
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})
