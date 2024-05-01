cmake_minimum_required(VERSION 3.8)
project(MLRI)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)
#set(ICON res/glfw.rc)

include_directories(dir/include)
link_directories(dir/lib)

link_libraries(glfw3)
link_libraries(ws2_32)
link_libraries(opengl32)
link_libraries(GLEW32)
link_libraries(GLU32)

file(GLOB CPP_FILES ./*.cpp)
file(GLOB HEADER_FILES ./*.h)

add_executable(mla_r ${CPP_FILES} ${HEADER_FILES})