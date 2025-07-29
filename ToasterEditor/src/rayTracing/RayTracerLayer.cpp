#include "RayTracerLayer.hpp"
#include "../util/Random.hpp"

namespace tst
{
	
	RayTracerLayer::RayTracerLayer() : m_PerspectiveCameraCtrl(90.0f, 1.77f)
	{

	}

	void RayTracerLayer::loadAssets()
	{
		TextureParams defaultParams{};
		defaultParams.minFilter = TextureFiltering::LinearMipmapLinear;
		defaultParams.magFilter = TextureFiltering::Linear;
		defaultParams.wrapS = TextureWrapping::Repeat;
		defaultParams.wrapT = TextureWrapping::Repeat;
		defaultParams.generateMipmaps = true;

		TextureParams pixelTextureParams{};
		pixelTextureParams.minFilter = TextureFiltering::LinearMipmapNearest;
		pixelTextureParams.magFilter = TextureFiltering::Nearest;
		pixelTextureParams.wrapS = TextureWrapping::Repeat;
		pixelTextureParams.wrapT = TextureWrapping::Repeat;
		pixelTextureParams.generateMipmaps = true;

	}

	void RayTracerLayer::onAttach()
	{
		FramebufferCreateInfo framebufferCreateInfo{ 1280, 720 };
		m_Framebuffer = Framebuffer::create(framebufferCreateInfo);



		auto& vX = m_Framebuffer->getInfo().width;
		auto& vY = m_Framebuffer->getInfo().height;


	}

	void RayTracerLayer::onUpdate(DeltaTime dt)
	{

		if (!Input::isMouseButtonPressed(TST_MOUSE_BUTTON_2))
		{
			Input::unfocusMouseCursor();
			return;
		}

		Input::focusMouseCursor();

		m_PerspectiveCameraCtrl.onUpdate(dt);

	}
	void RayTracerLayer::onEvent(Event& e)
	{
		
	}

	bool RayTracerLayer::onKeyPressedEvent(KeyPressedEvent& e)
	{
		return false;
	}

	uint32_t vecToUintRGBA(const glm::vec4& colour)
	{
		uint8_t r = static_cast<uint8_t>(colour.r * 255.0f);
		uint8_t g = static_cast<uint8_t>(colour.g * 255.0f);
		uint8_t b = static_cast<uint8_t>(colour.b * 255.0f);
		uint8_t a = static_cast<uint8_t>(colour.a * 255.0f);

		return (a << 24) | (b << 16) | (g << 8) | r;
	}

	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		void Reset()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		float Elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
		}

		float ElapsedMillis()
		{
			return Elapsed() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

	class ScopedTimer
	{
	public:
		ScopedTimer(const std::string& name)
			: m_Name(name) {
		}
		~ScopedTimer()
		{
			float time = m_Timer.ElapsedMillis();
			std::cout << "[TIMER] " << m_Name << " - " << time << "ms\n";
		}
	private:
		std::string m_Name;
		Timer m_Timer;
	};

	void RayTracerLayer::render()
	{

		ScopedTimer timer("RayTracerLayer::render");


		if (m_ViewportWidth == 0 || m_ViewportHeight == 0) {
			TST_CORE_WARN("RayTracerLayer::render: Invalid viewport dimensions [{0} x {1}]", m_ViewportWidth, m_ViewportHeight);
			return;
		}

		if (!m_renderTexture || m_ViewportWidth != m_renderTexture->getWidth() || m_ViewportHeight != m_renderTexture->getHeight())
		{
			TST_CORE_INFO("RayTracerLayer: Creating new texture {0}x{1}", m_ViewportWidth, m_ViewportHeight);

			TextureParams texture_params{};
			texture_params.pixelDataType = TexturePixelDataType::UnsignedByte;
			texture_params.minFilter = TextureFiltering::Nearest;
			texture_params.magFilter = TextureFiltering::Nearest;
			texture_params.generateMipmaps = false;
			texture_params.wrapS = TextureWrapping::ClampToEdge;
			texture_params.wrapT = TextureWrapping::ClampToEdge;

			m_renderTexture = Texture2D::create(m_ViewportWidth, m_ViewportHeight, texture_params);

			if (!m_renderTexture) {
				TST_CORE_ERROR("Failed to create render texture!");
				return;
			}

			delete[] m_imageData;
			m_imageData = new uint32_t[m_ViewportWidth * m_ViewportHeight];

			TST_CORE_INFO("Texture created successfully with ID: {0}", m_renderTexture->getId());
		}

		for (uint32_t y = 0; y < m_renderTexture->getHeight(); y++)
		{
			for (uint32_t x = 0; x < m_renderTexture->getWidth(); x++)
				{
					glm::vec2 coord = { static_cast<float>(x) / static_cast<float>(m_renderTexture->getWidth()), static_cast<float>(y) / static_cast<float>(m_renderTexture->getHeight()) };
					coord = coord * 2.0f - 1.0f;
					glm::vec4 colour = perPixel(coord);
					colour = glm::clamp(colour, glm::vec4(0.0f), glm::vec4(1.0f));
					m_imageData[x + y * m_renderTexture->getWidth()] = vecToUintRGBA(colour);
			}
		}

		// Upload the random data to the texture
		m_renderTexture->setData(m_imageData);
	}

	glm::vec4 RayTracerLayer::perPixel(glm::vec2 coord)
	{
		uint8_t r = (uint8_t)(coord.x * 255.0f);
		uint8_t g = (uint8_t)(coord.y * 255.0f);

		constexpr glm::vec3 rayOrigin	   = { -0.5f, 0.0f, -1.5f };
		const glm::vec3 rayDirection = glm::normalize(glm::vec3{ coord.x, coord.y, 1.0f });

		constexpr float sphereRadius = 0.5f;

		float a = glm::dot(rayDirection, rayDirection);
		float b = 2.0f * glm::dot(rayOrigin, rayDirection);
		float c = glm::dot(rayOrigin, rayOrigin) - (sphereRadius * sphereRadius);

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
		{
			return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}

		float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a);
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);

		glm::vec3 hitPoint = rayOrigin + rayDirection * closestT;
		glm::vec3 normal = glm::normalize(hitPoint);

		glm::vec3 lightDir = glm::normalize(glm::vec3{ -1.0f, -1.0f, 2.0f });

		float diff = glm::max(glm::dot(normal, -lightDir), 0.005f);

		glm::vec3 sphereColour = glm::vec3(0.0f, 0.32f, 0.62f);
		sphereColour *= diff;

		return glm::vec4(sphereColour, 1.0f);
	}


	void RayTracerLayer::onImguiRender()
	{
		static bool p_open = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", &p_open, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("Ray Tracer Dockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		ImGui::Begin("Render Settings");

		ImGui::Text("Yes, I am!");

		if (ImGui::Button("Render"))
		{
			render();
		}
		ImGui::End();

		ImGui::Begin("Viewport##1");

		ImVec2 viewportContentRegion = ImGui::GetContentRegionAvail();

		// Ensure viewport size is within reasonable bounds
		uint32_t newWidth = static_cast<uint32_t>(std::max(1.0f, std::min(4096.0f, viewportContentRegion.x)));
		uint32_t newHeight = static_cast<uint32_t>(std::max(1.0f, std::min(4096.0f, viewportContentRegion.y)));

		// Check if the viewport size has changed significantly
		if (abs(static_cast<int>(newWidth) - static_cast<int>(m_ViewportWidth)) > 1 ||
			abs(static_cast<int>(newHeight) - static_cast<int>(m_ViewportHeight)) > 1) {

			m_ViewportWidth = newWidth;
			m_ViewportHeight = newHeight;

			TST_CORE_INFO("Viewport size changed to: [{0} x {1}]", m_ViewportWidth, m_ViewportHeight);
		}

		if ((m_renderTexture) && (m_ViewportWidth > 0) && (m_ViewportHeight > 0))
		{
			ImGui::Image(m_renderTexture->getId(), { static_cast<float>(m_renderTexture->getWidth()), static_cast<float>(m_renderTexture->getHeight()) }, ImVec2(0, 1), ImVec2(1, 0));
		}
		else {
			ImGui::Text("No texture available or invalid viewport size");
		}

		ImGui::End();

		ImGui::End();

	}
}