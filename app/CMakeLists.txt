cmake_minimum_required(VERSION 3.4)
project(debugtocpp_app)

set(SOURCE_FILES src/debugextract.cpp)
add_executable(debugtocpp_app ${SOURCE_FILES})

target_include_directories(debugtocpp_app PUBLIC include)
target_link_libraries(debugtocpp_app debugtocpp_lib)