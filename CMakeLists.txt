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
        src/TinygltfImpl.cpp
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
        src/vulkan/ImguiVulkan.cpp
        src/vulkan/ImguiVulkan.h
        src/vulkan/GltfSceneVulkan.cpp
        src/vulkan/GltfSceneVulkan.h
        src/vulkan/GltfSceneVulkan.cpp
        src/vulkan/SamplerPool.h
        src/vulkan/SamplerPool.cpp
        src/vulkan/StagingUploader.h
        src/vulkan/StagingUploader.cpp
        src/vulkan/RtPipeline.cpp
        src/vulkan/RtPipeline.h
        src/vulkan/GltfSceneRt.cpp
        src/vulkan/GltfSceneRt.h
)

# Shaders
function(add_slang_shader TARGET SOURCE)
    cmake_parse_arguments(SHADER "" "PROFILE" "ENTRIES" ${ARGN})

    if(NOT SHADER_PROFILE)
        set(SHADER_PROFILE spirv_1_4)
    endif()

    get_filename_component(FILE_NAME "${SOURCE}" NAME_WE)

    set(OUTPUT_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    )

    set(OUTPUT_FILE
            "${OUTPUT_DIR}/${FILE_NAME}.spv"
    )

    add_custom_command(
            OUTPUT "${OUTPUT_FILE}"

            COMMAND
            ${CMAKE_COMMAND} -E make_directory
            "${OUTPUT_DIR}"

            COMMAND
            ${SLANGC_EXECUTABLE}
            "${SOURCE}"
            -target spirv
            -profile "${SHADER_PROFILE}"
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            ${SHADER_ENTRIES}
            -o "${OUTPUT_FILE}"

            DEPENDS "${SOURCE}"

            COMMENT
            "Compiling ${SOURCE} -> ${OUTPUT_FILE}"

            VERBATIM
    )

    add_custom_target(
            ${TARGET}
            DEPENDS "${OUTPUT_FILE}"
    )
endfunction()

add_slang_shader(
        ${PROJECT_NAME}_raster_shaders

        "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/raster.slang"

        ENTRIES
        -entry vertMain
        -entry fragMain

        PROFILE
        spirv_1_4
)

add_slang_shader(
        ${PROJECT_NAME}_rt_shaders

        "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/pathtrace.slang"

        ENTRIES
        -entry raygenMain
        -entry missMain
        -entry closestHitMain

        PROFILE
        spirv_1_4
)

target_include_directories(${PROJECT_NAME} PRIVATE "./vendor")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/assets
)

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_rt_shaders ${PROJECT_NAME}_raster_shaders)

# Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm::glm)
