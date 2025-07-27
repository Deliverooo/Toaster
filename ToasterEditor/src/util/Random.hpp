#pragma once
#include <random>

#include "Toaster/Core/Core.hpp"

namespace tst
{
	class TST_API Random
	{
	public:
		static std::random_device rd;
		static std::mt19937 gen;

		static int Int();
		static int Int(int min, int max);

		static uint32_t Uint();
		static uint32_t Uint(uint32_t min, uint32_t max);

		static float Float();
		static float Float(float min, float max);

		static double Double();
		static double Double(double min, double max);


		template<typename T>
		T* Array(uint32_t size, T min, T max);
	};
}
