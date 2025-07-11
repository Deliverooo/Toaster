#include "tstpch.h"
#include "ImguiLayer.hpp"
#include "imgui.h"
#include "GLFW/glfw3.h"
#include "Toaster/Application.hpp"
#include "Toaster/platform/OpenGl/ImguiOpenGlRenderer.h"

namespace tst
{
	ImguiLayer::ImguiLayer()
	{
		
	}

	ImguiLayer::~ImguiLayer()
	{
		
	}

	void ImguiLayer::onAttach()
	{
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void ImguiLayer::onDetach()
	{
		
	}

	void ImguiLayer::onUpdate()
	{

		ImGuiIO& io = ImGui::GetIO();
		auto& app = Application::getInstance();
		io.DisplaySize = {(float)app.getWindow().getWidth(), (float)app.getWindow().getHeight()};

		float time = glfwGetTime();
		io.DeltaTime = m_time > 0.0f ? (time - m_time) : (1.0f / 30.0f);
		m_time = time;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();


		bool show = true;
		ImGui::ShowDemoWindow(&show);


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	}

	void ImguiLayer::onEvent(Event& e)
	{
		
	}

}
