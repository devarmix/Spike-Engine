#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace ShaderCompiler {

	class Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
	};
}

// Core log macros
#define COMPILER_ERROR(...)    ::ShaderCompiler::Log::GetCoreLogger()->error(__VA_ARGS__);
#define COMPILER_WARN(...)     ::ShaderCompiler::Log::GetCoreLogger()->warn(__VA_ARGS__);
#define COMPILER_INFO(...)     ::ShaderCompiler::Log::GetCoreLogger()->info(__VA_ARGS__);
#define COMPILER_TRACE(...)    ::ShaderCompiler::Log::GetCoreLogger()->trace(__VA_ARGS__);
#define COMPILER_FATAL(...)    ::ShaderCompiler::Log::GetCoreLogger()->critical(__VA_ARGS__);