cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

project(AhnrealEngine_VK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(assimp REQUIRED)
find_package(Stb REQUIRED)

# Enable validation layers in debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

# Source files
set(ENGINE_CORE_SOURCES
    src/Engine/Core/Application.cpp
)

set(ENGINE_RENDERER_SOURCES
    src/Engine/Renderer/VulkanDevice.cpp
    src/Engine/Renderer/VulkanSwapChain.cpp
    src/Engine/Renderer/VulkanRenderer.cpp
)

set(ENGINE_SCENE_SOURCES
    src/Engine/Scene/Scene.cpp
)

set(ENGINE_UI_SOURCES
    src/Engine/UI/UISystem.cpp
)

set(SCENE_SOURCES
    src/Scenes/Basic/TriangleScene.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${ENGINE_CORE_SOURCES}
    ${ENGINE_RENDERER_SOURCES}
    ${ENGINE_SCENE_SOURCES}
    ${ENGINE_UI_SOURCES}
    ${SCENE_SOURCES}
)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Vulkan::Vulkan 
    glfw 
    glm::glm
    imgui::imgui
    assimp::assimp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})

# Shader compilation
find_program(GLSL_VALIDATOR glslangValidator HINTS 
    $ENV{VULKAN_SDK}/Bin 
    $ENV{VULKAN_SDK}/Bin32
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
    PATHS ${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/tools/vulkan
)

# Temporarily disable automatic shader compilation
# file(GLOB_RECURSE GLSL_SOURCE_FILES
#     "${PROJECT_SOURCE_DIR}/src/Shaders/*.frag"
#     "${PROJECT_SOURCE_DIR}/src/Shaders/*.vert"
#     "${PROJECT_SOURCE_DIR}/src/Shaders/*.comp"
# )

# foreach(GLSL ${GLSL_SOURCE_FILES})
#   get_filename_component(FILE_NAME ${GLSL} NAME)
#   set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
#   add_custom_command(
#     OUTPUT ${SPIRV}
#     COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
#     COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
#     DEPENDS ${GLSL})
#   list(APPEND SPIRV_BINARY_FILES ${SPIRV})
# endforeach(GLSL)

# add_custom_target(
#     shaders 
#     DEPENDS ${SPIRV_BINARY_FILES}
# )

# add_dependencies(${PROJECT_NAME} shaders)

# Create shaders directory in source
file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/src/Shaders")