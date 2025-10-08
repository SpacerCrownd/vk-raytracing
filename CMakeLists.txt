cmake_minimum_required (VERSION 3.31)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_STANDARD 20)

project (vk-raytracing)

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
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)
# Add the vulkan.cppm file directly as a source file
target_sources(VulkanCppModule
        PRIVATE
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

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

# assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v6.0.2
)
FetchContent_MakeAvailable(assimp)

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor)

add_executable(${PROJECT_NAME}
        src/Pathtracer/Application.cpp
        src/Vulkan/Renderer.cpp
        src/Vulkan/VulkanUtils.cpp
        src/Vulkan/VulkanDevice.cpp
        src/Vulkan/vk_mem_alloc.cpp
        src/Vulkan/VulkanQueue.cpp
        src/Pathtracer/Model.cpp
        src/Pathtracer/Model.h
        src/Vulkan/VulkanShader.cpp
        src/Vulkan/VulkanShader.h
        src/Pathtracer/Scene.cpp
        src/Pathtracer/Scene.h
        src/Vulkan/VulkanSwapchain.cpp
        src/Vulkan/VulkanSwapchain.h
        src/Vulkan/VulkanWindow.cpp
        src/Vulkan/VulkanWindow.h
        src/main.cpp
        src/Pathtracer/Application.h
        src/Vulkan/VulkanPhysicalDevice.cpp
        src/Vulkan/VulkanPhysicalDevice.h
)

# Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::cppm glfw glm::glm assimp)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

function(add_slang_shader_target TARGET)
    cmake_parse_arguments(SHADER "" "PROFILE" "SOURCES;ENTRIES" ${ARGN})

    if(NOT SHADER_SOURCES)
        message(FATAL_ERROR "No SOURCES given to add_slang_shader_target")
    endif()

    if(NOT SHADER_ENTRIES)
        message(FATAL_ERROR "No ENTRIES given to add_slang_shader_target")
    endif()

    if(NOT SHADER_PROFILE)
        set(SHADER_PROFILE spirv_1_4) # default
    endif()

    set(SHADER_OUTPUTS "")

    foreach(SHADER_FILE ${SHADER_SOURCES})
        get_filename_component(FILE_NAME ${SHADER_FILE} NAME_WE)
        set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.spv)

        add_custom_command(
                OUTPUT ${OUTPUT_FILE}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
                COMMAND ${SLANGC_EXECUTABLE} ${SHADER_FILE}
                -target spirv
                -profile ${SHADER_PROFILE}
                -emit-spirv-directly
                -fvk-use-entrypoint-name
                ${SHADER_ENTRIES}
                -o ${OUTPUT_FILE}
                DEPENDS ${SHADER_FILE}
                COMMENT "Compiling ${SHADER_FILE} -> ${OUTPUT_FILE}"
                VERBATIM
        )

        list(APPEND SHADER_OUTPUTS ${OUTPUT_FILE})
    endforeach()

    add_custom_target(${TARGET} DEPENDS ${SHADER_OUTPUTS})
endfunction()
