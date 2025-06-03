project "VK-Bootstrap"
    location "%{wks.location}/Source/ThirdParty/vk-bootstrap"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("%{wks.location}/Binaries/" .. outputDir .. "/%{prj.name}")
	objdir ("%{wks.location}/Intermediate/" .. outputDir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{IncludeDir.VULKAN_SDK}"
	}

	filter("system:windows")
		systemversion "latest"

	filter "configurations:Debug"
		defines "ENGINE_BUILD_DEBUG"
		symbols "on"

	filter "configurations:Release"
		defines "ENGINE_BUILD_RELEASE"
		optimize "on"