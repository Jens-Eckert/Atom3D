cmake_minimum_required(VERSION 3.19...3.31)

project(Atom3D)

message("CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}") 

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

file(GLOB SOURCES "src/*.cpp")

add_executable(main ${SOURCES})

target_include_directories(main PUBLIC include)
target_compile_features(main PUBLIC cxx_std_17)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

target_link_libraries(main Vulkan::Vulkan)
target_link_libraries(main glfw)
target_link_libraries(main glm::glm)

#execute_process(COMMAND glslc ../src/shaders/shader.vert -o ../src/shaders/vert.spv)
#execute_process(COMMAND glslc ../src/shaders/shader.frag -o ../src/shaders/frag.spv)