cmake_minimum_required (VERSION 3.31)

project (vk-raytracing)
set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

# Vulkan
find_package (Vulkan REQUIRED)

# set up Vulkan C++ module
add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_compile_definitions(VulkanCppModule PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)
target_include_directories(VulkanCppModule
        PRIVATE
        "${Vulkan_INCLUDE_DIR}"
)
target_link_libraries(VulkanCppModule
        PUBLIC
        Vulkan::Vulkan
)

set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

target_sources(VulkanCppModule
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS
        "${Vulkan_INCLUDE_DIR}"
        FILES
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm")

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

#tinyobjloader
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader
    GIT_TAG v2.0-rc1
)
FetchContent_MakeAvailable(tinyobjloader)

add_executable(vk-raytracing "src/Application.cpp" "src/Vulkan/Core/VulkanCore.cpp" "src/vk_rt_utils.cpp")

# Include dirs
target_include_directories(vk-raytracing PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Libraries
target_link_libraries(vk-raytracing PRIVATE Vulkan::cppm glfw glm::glm tinyobjloader)
