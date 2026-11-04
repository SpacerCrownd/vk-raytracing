# vk-raytracer
A Vulkan-based Monte Carlo path tracer featuring glTF 2.0 scene loading and physically based rendering using the metallic-roughness workflow. 
The renderer uses the GGX microfacet BRDF for specular lighting and Lambertian diffuse lighting, with Visible Normal Distribution Function (VNDF) sampling for specular reflections and cosine-weighted hemisphere sampling for diffuse reflections.
Only delta-distribution light sources (punctual, directional and spotlight) are currently supported via Next Event Estimation.

# Features
- Next Event Estimation
- Russian Roulette
- glTF 2.0 scene loading
- Scene transform editing
- glTF punctual lights
- Dear ImGui user interface
- CPU denoising with Intel Open Image Denoise

# Requirements
- Vulkan 1.3 or later
- ReBAR/SAM
- Raytracing capable GPU
- CMake 3.14 or later
- Open Image Denoise (OIDN)
- Slang compiler

# Building and Execution
1. Install the Intel Open Image Denoise binaries into vendor/OpenImageDenoise/bin.
2. Install the Slang compiler and ensure it is available in your system PATH.
3. Configure and build the project using CMake.
4. Run the generated executable.

# References
https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/tutorial/
https://www.howtovulkan.com/
https://vkguide.dev/
https://docs.vulkan.org/tutorial/latest/00_Introduction.html
https://github.com/nvpro-samples/vk_gltf_renderer
https://github.com/RenderKit/oidn
https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md
https://github.khronos.org/glTF-Tutorials/gltfTutorial/