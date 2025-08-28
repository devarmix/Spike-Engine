#include <iostream>

#include <Core/Compiler.h>
#include <Core/Log.h>

// spirv-reflect build file
#include <spirv_reflect.cpp>

int main() {

	ShaderCompiler::Log::Init();
	ShaderCompiler::CompileShaders(
		"C:/Users/Artem/Desktop/Spike-Engine/Resources/Hlsl-Shaders", 
		"C:/Users/Artem/Desktop/Spike-Engine/Resources/Hlsl-Shaders/Cache", 
		"C:/Users/Artem/Desktop/Spike-Engine/Resources/Hlsl-Shaders/Include/Generated");

	return 0;
}