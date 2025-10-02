project "ImGui"
    location "%{wks.location}/Source/ThirdParty/imgui"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	targetdir ("%{wks.location}/Binaries/" .. outputDir .. "/%{prj.name}")
	objdir ("%{wks.location}/Intermediate/" .. outputDir .. "/%{prj.name}")

	files
	{
		"include/imgui/imconfig.h",
		"include/imgui/imgui.h",
		"include/imgui/imgui.cpp",
		"include/imgui/imgui_draw.cpp",
		"include/imgui/imgui_internal.h",
		"include/imgui/imgui_widgets.cpp",
		"include/imgui/imgui_tables.cpp",
		"include/imgui/imstb_rectpack.h",
		"include/imgui/imstb_textedit.h",
		"include/imgui/imstb_truetype.h",
		"include/imgui/imgui_demo.cpp"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
	    defines "ENGINE_BUILD_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
	    defines "ENGINE_BUILD_RELEASE"
		runtime "Release"
		optimize "on"