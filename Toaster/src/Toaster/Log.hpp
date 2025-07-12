#pragma once

#include "Core.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"



namespace tst {

	using TstResult = int;

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

#ifdef TST_DEBUG
#define TST_CORE_ERROR(...)		 tst::Log::getCoreLogger()->error(__VA_ARGS__)
#define TST_CORE_WARN(...)		 tst::Log::getCoreLogger()->warn(__VA_ARGS__)
#define TST_CORE_INFO(...)		 tst::Log::getCoreLogger()->info(__VA_ARGS__)
#define TST_CORE_TRACE(...)		 tst::Log::getCoreLogger()->trace(__VA_ARGS__)

#define TST_ERROR(...)	 tst::Log::getClientLogger()->error(__VA_ARGS__)
#define TST_WARN(...)	 tst::Log::getClientLogger()->warn(__VA_ARGS__)
#define TST_INFO(...)	 tst::Log::getClientLogger()->info(__VA_ARGS__)
#define TST_TRACE(...)	 tst::Log::getClientLogger()->trace(__VA_ARGS__)

#define TST_ASSERT(expr, msg)	if(!(expr)){ tst::Log::getCoreLogger()->error(##msg); __debugbreak();}
#define TST_BREAKPOINT() tst::Log::getCoreLogger()->trace("Breakpoint Reached"); __debugbreak()

#else
#define TST_CORE_ERROR(...)
#define TST_CORE_WARN(...)
#define TST_CORE_INFO(...)
#define TST_CORE_TRACE(...)

#define TST_ERROR(...)
#define TST_WARN(...)
#define TST_INFO(...)
#define TST_TRACE(...)

#define TST_ASSERT(expr, msg)
#define TST_BREAKPOINT()


#endif