cmake_minimum_required (VERSION 3.14 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 23)

project (vk-raytracing)

include(FetchContent)

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
        src/Pathtracer/Renderer.cpp
        src/Vulkan/VulkanCore.cpp
        src/Vulkan/VulkanUtils.cpp
        src/Vulkan/VulkanDevice.cpp
        src/Vulkan/vk_mem_alloc.cpp
        src/Vulkan/Model.cpp
        src/Vulkan/Model.h
        src/Vulkan/VulkanShader.cpp
        src/Vulkan/VulkanShader.h
        src/Vulkan/Scene.cpp
        src/Vulkan/Scene.h
        src/Vulkan/VulkanSwapchain.cpp
        src/Vulkan/VulkanSwapchain.h
        src/Vulkan/VulkanWindow.cpp
        src/Vulkan/VulkanWindow.h
        src/main.cpp
        src/Pathtracer/Renderer.h
        src/Vulkan/VulkanPhysicalDevice.cpp
        src/Vulkan/VulkanPhysicalDevice.h
        src/Pathtracer/Camera.cpp
        src/Pathtracer/Camera.h
        src/Vulkan/Commands.cpp
        src/Vulkan/Commands.h
        src/Vulkan/Resources.h
        vendor/Tinyobjloader/tiny_obj_loader.h
        src/Vulkan/ResourceAllocator.cpp
        src/Vulkan/ResourceAllocator.h
)

# Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm::glm ${Slang_LIBRARY})

target_include_directories(${PROJECT_NAME} PRIVATE "./vendor")

add_custom_target(copy_assets COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy_assets.cmake)

add_dependencies(${PROJECT_NAME} copy_assets)

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
