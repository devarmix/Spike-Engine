project "EngineCore"
    location "%{wks.location}/Source/EngineCore"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
	staticruntime "on"

    targetdir ("%{wks.location}/Binaries/" .. outputDir .. "/%{prj.name}")
	objdir ("%{wks.location}/Intermediate/" .. outputDir .. "/%{prj.name}")

	files
	{
		"**.h",
		"**.cpp"
	}

	includedirs
	{
        "%{IncludeDir.SPDLOG}",
        "%{IncludeDir.STB_IMAGE}",
        "%{IncludeDir.GLM}",
		"%{IncludeDir.IMGUI}",
        "%{IncludeDir.SDL}",
        "%{IncludeDir.VMA}",
        "%{IncludeDir.VK_BOOTSTRAP}",
		"%{IncludeDir.VULKAN_SDK}",
		"%{IncludeDir.FAST_GLTF}",
		"%{IncludeDir.ENTT}",
		"%{IncludeDir.SHADER_COMPILER}",
		"%{IncludeDir.SHADERS_GENERATED}",
		""
	}

	links
	{
        "ImGUI",
		"VK-Bootstrap",
		"FastGLTF",

		-- SDL libs
		"%{LibsDir.SDL}/x64/SDL2.lib",
		"%{LibsDir.SDL}/x64/SDL2main.lib",

		-- Vulkan libs
		"%{LibsDir.VULKAN_SDK}/vulkan-1.lib"
	}

	filter("system:windows")
		systemversion "latest"
		buildoptions "/utf-8"

		defines
		{
			"ENGINE_PLATFORM_WINDOWS",
			"GLM_FORCE_DEPTH_ZERO_TO_ONE"
		}

	filter "configurations:Debug"
		defines "ENGINE_BUILD_DEBUG"
		symbols "on"

	filter "configurations:Release"
		defines "ENGINE_BUILD_RELEASE"
		optimize "on"

	filter "configurations:Distribution"
		defines "ENGINE_BUILD_DISTRIBUTION"
		optimize "on"