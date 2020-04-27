cmake_minimum_required(VERSION 3.17)

project(Viper VERSION 0.1.0 DESCRIPTION "Viper the opensource video editor" LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

add_executable(Viper src/main.cpp)

find_package(Vulkan REQUIRED)

if(Vulkan_FOUND)
    if(WIN32)
        target_compile_definitions(Viper PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    elseif(UNIX)
        # TODO: Wayland support
        target_compile_definitions(Viper PRIVATE VK_USE_PLATFORM_XCB_KHR)
    endif()
    target_include_directories(Viper PRIVATE Vulkan::Vulkan)
    target_link_libraries(Viper PRIVATE Vulkan::Vulkan)
endif()

add_subdirectory(libs/glfw EXCLUDE_FROM_ALL)
target_link_libraries(Viper PRIVATE glfw)

add_subdirectory(libs/glm EXCLUDE_FROM_ALL)
target_link_libraries(Viper PRIVATE glm)
