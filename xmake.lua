add_rules("mode.debug", "mode.release")

add_requires("vulkan-headers", "vulkan-loader","volk")
add_requires("shaderc")
add_requires("libsdl3")
add_requires("glm")

target("test")
set_kind("binary")
add_includedirs("src/include")
add_files("src/lib/*.cpp")
add_files("src/main.cpp")
add_packages("vulkan-headers", "vulkan-loader", "glm", "libsdl3", "shaderc","volk")
set_languages("c++23")
