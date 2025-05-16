include "Dependencies.lua"

workspace "Spike-Engine" 
	architecture "x64"
	startproject "Spike-Editor"

	configurations
	{
		"Debug",
		"Release",
		"Distribution"
	}

outputDir = "%{cfg.build}-%{cfg.system}-%{cfg.architecture}"

-- Dependencies
include "Source/Third-Party/imgui/Build.lua"
include "Source/Third-Party/vk-bootstrap/Build.lua"
include "Source/Third-Party/fastgltf/Build.lua"

-- Core
include "Source/Engine-Core/Build.lua"
include "Source/Spike-Editor/Build.lua"