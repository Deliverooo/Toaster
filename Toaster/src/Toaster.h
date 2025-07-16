#pragma once

// Toaster Engine Main Header
// This header includes all the necessary headers for the Toaster engine.
// This should only be included in the client application.




// Toaster Engine Application Header
#include "Toaster/Application.hpp"

// Toaster Engine Layer Header
#include "Toaster/Layer.hpp"

// Toaster Engine ImGui Layer Header
#include "Toaster/imgui/ImguiLayer.hpp"

// Glm Maths library functions
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.inl>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

// Toaster Engine Renderer and Renderer Command Headers
#include "Toaster/Renderer/RenderCommand.hpp"
#include "Toaster/Renderer/Renderer.hpp"
#include "Toaster/Renderer/RendererAPI.hpp"
#include "Toaster/Renderer/Buffer.hpp"
#include "Toaster/Renderer/Camera.hpp"
#include "Toaster/Renderer/RenderingContext.hpp"
#include "Toaster/Renderer/Shader.hpp"
#include "Toaster/Renderer/Texture.hpp"
#include "Toaster/Renderer/VertexArray.hpp"

// Toaster Engine Input Header
#include "Toaster/Input.hpp"

// Toaster Engine KeyCodes and MouseCodes Headers
#include "Toaster/KeyCodes.hpp"
#include "Toaster/MouseCodes.hpp"

// Toaster Engine OpenGl Test Layer Header
#include "platform/OpenGl/OpenGlTestLayer.h"

// Toaster Engine Log Wrapper Header
#include "Toaster/Log.hpp"

// Toaster Engine Entry Point
#include "Toaster/EntryPoint.hpp"