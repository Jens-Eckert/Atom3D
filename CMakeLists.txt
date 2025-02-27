cmake_minimum_required(VERSION 3.19...3.31)

project(Atom3D)

if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "D:/dev/glfw-3.4/install/lib/cmake/glfw3")
endif()

list(APPEND CMAKE_PREFIX_PATH "~/dev/VulkanMemoryAllocator/build/install")
list(APPEND CMAKE_PREFIX_PATH "~/dev/VulkanMemoryAllocator-Hpp/build/install")
message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}") 

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

file(GLOB SOURCES "src/*.cpp")

add_executable(main ${SOURCES})

target_include_directories(main PUBLIC include)
target_compile_features(main PUBLIC cxx_std_17)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(VulkanMemoryAllocator-Hpp REQUIRED)

target_link_libraries(main Vulkan::Vulkan)
target_link_libraries(main glfw)
target_link_libraries(main glm::glm)
target_link_libraries(main VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp GPUOpen::VulkanMemoryAllocator)