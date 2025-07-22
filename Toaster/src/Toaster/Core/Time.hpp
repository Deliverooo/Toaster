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
}