#pragma once

namespace tst
{
	enum class RenderApi
	{
		None, OpenGL, Vulkan, DirectX,
	};

	class TST_API Renderer
	{
	public:

		static Renderer& getInstance() { return m_instance; }
		static RenderApi getApi() { return m_renderApi; }

	private:
		static Renderer m_instance;
		static RenderApi m_renderApi;
	};
}
