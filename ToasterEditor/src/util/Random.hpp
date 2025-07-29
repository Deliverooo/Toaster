#pragma once
#include <random>

#include "Toaster/Core/Core.hpp"

namespace tst
{
	namespace Random
	{
		static std::random_device rd;
		static std::mt19937 gen;

		template<typename T>
		inline T Gen() { return 0; }

		template<typename T>
		inline T Gen(T min, T max) { return static_cast<T>(0); }

		template<>
		inline int Gen<int>()
		{
			uint32_t res = gen();
			return *((int*)&res);
		}

		template<>
		inline int Gen<int>(int min, int max)
		{
			std::uniform_int_distribution<int> dist(min, max);
			return dist(gen);
		}

		template<>
		inline uint32_t Gen<uint32_t>()	{ return gen(); }

		template<>
		inline uint32_t Gen<uint32_t>(uint32_t min, uint32_t max)
		{
			std::uniform_int_distribution<uint32_t> dist(min, max);
			return dist(gen);
		}

		template<>
		inline float Gen<float>() { return static_cast<float>(gen()) / static_cast<float>(std::mt19937::max()); }

		template<>
		inline float Gen<float>(float min, float max)
		{
			std::uniform_real_distribution<float> dist(min, max);
			return dist(gen);
		}

		template<>
		inline double Gen<double>() { return static_cast<double>(gen()) / static_cast<double>(std::mt19937::max());	}

		template<>
		inline double Gen<double>(double min, double max)
		{
			std::uniform_real_distribution<double> dist(min, max);
			return dist(gen);
		}

		template<>
		inline long Gen<long>() { return static_cast<long>(gen()); }

		template<>
		inline long Gen<long>(long min, long max)
		{
			std::uniform_int_distribution<long> dist(min, max);
			return dist(gen);
		}
	}
}
