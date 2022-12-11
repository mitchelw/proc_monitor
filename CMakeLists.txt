cmake_minimum_required(VERSION 3.5)
project(proc_monitor)

set(CMAKE_CXX_STANDARD 14)

add_executable(proc_monitor src/main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(proc_monitor pthread)



add_subdirectory(src/Simple-Web-Server-master)
target_link_libraries(simple-web-server INTERFACE ${CMAKE_THREAD_LIBS_INIT})
find_package(Boost 1.53.0 COMPONENTS system thread REQUIRED)
#set(BOOST_LIBRARYDIR ${BOOST_ROOT}/stage/lib/)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
target_link_libraries(simple-web-server INTERFACE ${Boost_LIBRARIES})
target_include_directories(simple-web-server INTERFACE ${Boost_INCLUDE_DIR})
#if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    target_compile_definitions(simple-web-server INTERFACE USE_BOOST_REGEX)
    find_package(Boost 1.53.0 COMPONENTS regex REQUIRED)
    target_link_libraries(simple-web-server INTERFACE ${Boost_LIBRARIES})
    target_include_directories(simple-web-server INTERFACE ${Boost_INCLUDE_DIR})
#endif()