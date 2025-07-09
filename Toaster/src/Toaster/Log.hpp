#pragma once

#include <memory>

#include "Core.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"


namespace tst {

	class TST_API Log {

	public:
		
		static void init();

		static std::shared_ptr<spdlog::logger>& getCoreLogger() { return m_coreLogger; }
		static std::shared_ptr<spdlog::logger>& getClientLogger() { return m_clientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> m_coreLogger;
		static std::shared_ptr<spdlog::logger> m_clientLogger;

	};
}

#define TST_CORE_ERROR(...)		 tst::Log::getCoreLogger()->error(__VA_ARGS__)
#define TST_CORE_WARN(...)		 tst::Log::getCoreLogger()->warn(__VA_ARGS__)
#define TST_CORE_INFO(...)		 tst::Log::getCoreLogger()->info(__VA_ARGS__)
#define TST_CORE_TRACE(...)		 tst::Log::getCoreLogger()->trace(__VA_ARGS__)

#define TST_ERROR(...)	 tst::Log::getClientLogger()->error(__VA_ARGS__)
#define TST_WARN(...)	 tst::Log::getClientLogger()->warn(__VA_ARGS__)
#define TST_INFO(...)	 tst::Log::getClientLogger()->info(__VA_ARGS__)
#define TST_TRACE(...)	 tst::Log::getClientLogger()->trace(__VA_ARGS__)