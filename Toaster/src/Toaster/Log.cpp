#include "Log.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace tst {

	std::shared_ptr<spdlog::logger> Log::m_coreLogger;
	std::shared_ptr<spdlog::logger> Log::m_clientLogger;

	void Log::init() {
		spdlog::set_pattern("%^[%I] %n: %v%$");
		m_coreLogger = spdlog::stdout_color_mt("Toaster");
		m_coreLogger->set_level(spdlog::level::trace);

		m_clientLogger = spdlog::stdout_color_mt("App");
		m_clientLogger->set_level(spdlog::level::trace);
	}
}