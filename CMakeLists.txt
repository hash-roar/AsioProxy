set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd

# init 
cmake_minimum_required(VERSION 3.1)
project(AsioProxy)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#add proxy library
aux_source_directory(./proxy SRCS)
add_library(proxy STATIC ${SRCS})
target_include_directories(proxy PUBLIC ${PROJECT_SOURCE_DIR}/proxy)
target_include_directories(proxy PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(proxy pthread)

# add excutable server
add_executable(server main.cc)
target_link_libraries(server proxy)
