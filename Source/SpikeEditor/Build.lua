project "SpikeEditor"
    location "%{wks.location}/Source/SpikeEditor"
    kind "ConsoleApp"
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
        "%{IncludeDir.VKBootstrap}",
		"%{IncludeDir.VULKAN_SDK}",
		"%{IncludeDir.FAST_GLTF}",
		"%{IncludeDir.ENGINE_CORE}",
		"%{IncludeDir.ENTT}",
		"%{IncludeDir.SHADER_COMPILER}",
		"%{IncludeDir.SHADERS_GENERATED}",
		""
	}

	links
	{
       "EngineCore"
	}

	filter("system:windows")
		systemversion "latest"
		buildoptions "/utf-8"

		defines
		{
			"ENGINE_PLATFORM_WINDOWS",
			"GLM_FORCE_DEPTH_ZERO_TO_ONE"
		}

		prebuildcommands 
		{
			("%{wks.location}/Binaries/" .. outputDir .. "/ShaderCompiler/ShaderCompiler.exe")
		}

		postbuildcommands 
		{
			("{COPY} %{LibsDir.SDL}/x64/SDL2.dll %{wks.location}/Binaries/" .. outputDir .. "/%{prj.name}")
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