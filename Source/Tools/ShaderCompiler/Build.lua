project "ShaderCompiler"
    location "%{wks.location}/Source/Tools/ShaderCompiler"
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
		"%{IncludeDir.DXC}",
		"",
		"Include"
	}

	links
	{
		-- dxc libs
		"%{LibsDir.DXC}/x64/dxcompiler.lib"
	}

	filter("system:windows")
		systemversion "latest"
		buildoptions "/utf-8"

		defines
		{
			"ENGINE_PLATFORM_WINDOWS"
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