set vulkan_sdk_path=C:\VulkanSDK\1.2.135.0
set glfw_path=C:\glfw-3.3.2.bin.WIN64
set glm_path=C:\glm-0.9.9.8

set cc=cl
set cflags=/std:c++17 /MD /I%vulkan_sdk_path%\include /I%glfw_path%\include /I%glm_path%
set ldflags=gdi32.lib kernel32.lib user32.lib shell32.lib %glfw_path%\lib-vc2019\glfw3.lib %vulkan_sdk_path%\lib\vulkan-1.lib
set sources=src\main.cpp

%cc% %sources% %cflags% %ldflags%
