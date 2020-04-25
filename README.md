# Viper
A fast, free and open-source Vulkan video editor for all operating systems.

## Building
### Libraries in Use

To do ...

### Steps
    git clone --recursive git@github.com:codemessiah/viper.git to download all the dependencies
One extra thing to get: 
1. Vulkan SDK by LunarG from https://vulkan.lunarg.com/sdk/home and install it for your operating system.

then you can run the default cmake commands to get going
```bash
mkdir build
cd build
cmake ..
cmake --build .
cd Debug
./Viper.exe
```    
*Note: some installation information for Vulkan, GLFW, and GLM can be found here if you need more help: https://vulkan-tutorial.com/Development_environment *
