project "FastGLTF"
    location "%{wks.location}/Source/ThirdParty/fastgltf"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("%{wks.location}/Binaries/" .. outputDir .. "/%{prj.name}")
	objdir ("%{wks.location}/Intermediate/" .. outputDir .. "/%{prj.name}")

	files
	{
		"src/**.cpp",
		"include/**.hpp"
	}

	includedirs
	{
		"include"
	}

	filter("system:windows")
		systemversion "latest"

	filter "configurations:Debug"
		defines "ENGINE_BUILD_DEBUG"
		symbols "on"

	filter "configurations:Release"
		defines "ENGINE_BUILD_RELEASE"
		optimize "on"