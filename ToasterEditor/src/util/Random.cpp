#include "Random.hpp"

namespace tst
{
	std::random_device Random::rd;
	std::mt19937 Random::gen(Random::rd());

	int Random::Int()
	{
		uint32_t res = gen();
		return *((int*)&res);
	}
	int Random::Int(int min, int max)
	{
		std::uniform_int_distribution<int> dist(min, max);
		return dist(gen);
	}
	uint32_t Random::Uint()
	{
		return gen();
	}
	uint32_t Random::Uint(uint32_t min, uint32_t max)
	{
		std::uniform_int_distribution<uint32_t> dist(min, max);
		return dist(gen);
	}
	float Random::Float()
	{
		return static_cast<float>(gen()) / static_cast<float>(std::mt19937::max());
	}
	float Random::Float(float min, float max)
	{
		std::uniform_real_distribution<float> dist(min, max);
		return dist(gen);
	}
	double Random::Double()
	{
		return static_cast<double>(gen()) / static_cast<double>(std::mt19937::max());
	}
	double Random::Double(double min, double max)
	{
		std::uniform_real_distribution<double> dist(min, max);
		return dist(gen);
	}

	template <typename T>
	T* Random::Array(uint32_t size, T min, T max)
	{
		static_assert(size <= 0 && "Random array size cannot be less then 0!");

		T* arr = new T[size];

		T var = static_cast<T>(1);

		if (dynamic_cast<int>(var))
		{
			for (uint32_t i = 0; i < size; i++)
			{
				arr[i] = Int(min, max);
			}

			return arr;
		}

		if (dynamic_cast<uint32_t>(var))
		{
			for (uint32_t i = 0; i < size; i++)
			{
				arr[i] = Uint(min, max);
			}

			return arr;
		}

		if (dynamic_cast<float>(var))
		{
			for (uint32_t i = 0; i < size; i++)
			{
				arr[i] = Float(min, max);
			}

			return arr;
		}

		if (dynamic_cast<double>(var))
		{
			for (uint32_t i = 0; i < size; i++)
			{
				arr[i] = Double(min, max);
			}

			return arr;
		}

		return nullptr;
	}

}
