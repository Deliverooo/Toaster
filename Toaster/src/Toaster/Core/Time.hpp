#pragma once

namespace tst {

	struct TST_API DeltaTime
	{
	public:
		DeltaTime(float start, float end)
		{
			m_time = end - start;
		}

		[[nodiscard]] const float& getTime_s()  const { return m_time; }
		[[nodiscard]] const float& getTime_ms() const { return m_time * 1000.0f; }

		operator float() const { return m_time; }

	private:
		float m_time;
	};

	//class TST_API Time {
	//public:

	//	Time(double time = 0.0) : m_time(time) {

	//	}

	//	[[nodiscard]] const double& getTime_s()  const { return m_time; }
	//	[[nodiscard]] const double& getTime_ms() const { return m_time * 1000.0; }

	//private:

	//	double m_time;
	//	//std::chrono::time_point<std::chrono::steady_clock> m_beginTime;
	//	//std::chrono::time_point<std::chrono::steady_clock> m_endTime;
	//	//std::chrono::duration<double> m_deltaTine;
	//};
}