cmake_minimum_required (VERSION 3.14 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 23)

project (vk-raytracing)

include(FetchContent)

find_program(SLANGC_EXECUTABLE slangc REQUIRED)

# Vulkan
find_package (Vulkan REQUIRED)
add_compile_definitions(
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
        VULKAN_HPP_NO_MODULE
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

add_executable(${PROJECT_NAME}
        src/vulkan/Core.cpp
        src/vulkan/Utils.cpp
        src/vulkan/Commands.cpp
        src/vulkan/Device.cpp
        src/vulkan/Shader.cpp
        src/vulkan/Swapchain.cpp
        src/vulkan/Window.cpp
        src/vulkan/PhysicalDevice.cpp
        src/vulkan/ResourceAllocator.cpp
        src/main.cpp
        src/Renderer.cpp
        src/Camera.cpp
        src/GltfUtils.cpp
        src/GltfScene.cpp
        src/vulkan/GraphicsPipeline.cpp
        src/vulkan/GraphicsPipeline.h
)

# Shaders
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

    set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

    foreach(SHADER_FILE ${SHADER_SOURCES})
        get_filename_component(FILE_NAME ${SHADER_FILE} NAME_WE)
        set(OUTPUT_FILE ${SHADER_OUTPUT_DIR}/${FILE_NAME}.spv)

        add_custom_command(
                OUTPUT ${OUTPUT_FILE}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
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

set(SHADER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/testRaster.slang
)

add_slang_shader_target(
        ${PROJECT_NAME}_shaders

        SOURCES
            ${SHADER_SOURCES}

        ENTRIES
            -entry vertMain
            -entry fragMain

        PROFILE
            spirv_1_4
)

target_include_directories(${PROJECT_NAME} PRIVATE "./vendor")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/assets
)

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_shaders)

# Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm::glm)
