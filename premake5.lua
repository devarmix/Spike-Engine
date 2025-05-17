include "Dependencies.lua"

workspace "SpikeEngine" 
	architecture "x64"
	startproject "SpikeEditor"

	configurations
	{
		"Debug",
		"Release",
		"Distribution"
	}

outputDir = "%{cfg.build}-%{cfg.system}-%{cfg.architecture}"

-- Dependencies
include "Source/ThirdParty/imgui/Build.lua"
include "Source/ThirdParty/vk-bootstrap/Build.lua"
include "Source/ThirdParty/fastgltf/Build.lua"

-- Core
include "Source/EngineCore/Build.lua"
include "Source/SpikeEditor/Build.lua"