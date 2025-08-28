-- Include Directories relative to the root folder
IncludeDir = {}

IncludeDir["SPDLOG"] = "%{wks.location}/Source/ThirdParty/spdlog/include"
IncludeDir["GLM"] = "%{wks.location}/Source/ThirdParty/glm"
IncludeDir["STB_IMAGE"] = "%{wks.location}/Source/ThirdParty/stb_image/include"
IncludeDir["IMGUI"] = "%{wks.location}/Source/ThirdParty/imgui"
IncludeDir["SDL"] = "%{wks.location}/Source/ThirdParty/SDL2/include"
IncludeDir["VMA"] = "%{wks.location}/Source/ThirdParty/VMA/include"
IncludeDir["VK_BOOTSTRAP"] = "%{wks.location}/Source/ThirdParty/vk-bootstrap/src"
IncludeDir["FAST_GLTF"] = "%{wks.location}/Source/ThirdParty/fastgltf/include"
IncludeDir["ENGINE_CORE"] = "%{wks.location}/Source/EngineCore"
IncludeDir["ENTT"] = "%{wks.location}/Source/ThirdParty/entt/include"
IncludeDir["VULKAN_SDK"] = "%{wks.location}/Source/ThirdParty/Vulkan/Include"
IncludeDir["DXC"] = "%{wks.location}/Source/ThirdParty/DXC/include"
IncludeDir["SHADER_COMPILER"] = "%{wks.location}/Source/Tools/ShaderCompiler/Include"
IncludeDir["SHADERS_GENERATED"] = "%{wks.location}/Resources/Hlsl-Shaders/Include"
IncludeDir["SPIRV_REFLECT"] = "%{wks.location}/Source/ThirdParty/SPIRV-Reflect/include"


-- Libs Directories relative to the root folder
LibsDir = {}

LibsDir["SDL"] = "%{wks.location}/Source/ThirdParty/SDL2/libs"
LibsDir["VULKAN_SDK"] = "%{wks.location}/Source/ThirdParty/Vulkan/Lib"
LibsDir["DXC"] = "%{wks.location}/Source/ThirdParty/DXC/libs"