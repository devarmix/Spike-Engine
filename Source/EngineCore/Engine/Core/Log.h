#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Spike {

	class Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core log macros
#define ENGINE_ERROR(...)    ::Spike::Log::GetCoreLogger()->error(__VA_ARGS__);
#define ENGINE_WARN(...)     ::Spike::Log::GetCoreLogger()->warn(__VA_ARGS__);
#define ENGINE_INFO(...)     ::Spike::Log::GetCoreLogger()->info(__VA_ARGS__);
#define ENGINE_TRACE(...)    ::Spike::Log::GetCoreLogger()->trace(__VA_ARGS__);
#define ENGINE_FATAL(...)    ::Spike::Log::GetCoreLogger()->critical(__VA_ARGS__);


// Client log macros
#define CLIENT_ERROR(...)    ::Spike::Log::GetClientLogger()->error(__VA_ARGS__);
#define CLIENT_WARN(...)     ::Spike::Log::GetClientLogger()->warn(__VA_ARGS__);
#define CLIENT_INFO(...)     ::Spike::Log::GetClientLogger()->info(__VA_ARGS__);
#define CLIENT_TRACE(...)    ::Spike::Log::GetClientLogger()->trace(__VA_ARGS__);
#define CLIENT_FATAL(...)    ::Spike::Log::GetClientLogger()->critical(__VA_ARGS__);