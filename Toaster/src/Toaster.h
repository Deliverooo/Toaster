#pragma once

// Toaster Engine Main Header
// This header includes all the necessary headers for the Toaster engine.
// This should only be included in the client application.


#include "Toaster/Core/Core.hpp"

// Toaster Engine Application Header
#include "Toaster/Core/Application.hpp"

// Toaster Engine Layer Header
#include "Toaster/Core/Layer.hpp"

// Toaster Engine ImGui Layer Header
#include "Toaster/imgui/ImguiLayer.hpp"

// Glm Maths library functions
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.inl>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

// Toaster Engine Scene Header
#include "Toaster/Scene/Scene.hpp"
#include "Toaster/Scene/Entity.hpp"
#include "Toaster/Scene/ScriptableEntity.hpp"
#include "Toaster/Scene/Components.hpp"

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
#include "Toaster/Renderer/Material.hpp"
#include "Toaster/CameraController.hpp"
#include "Toaster/Renderer/Renderer2D.hpp"
#include "Toaster/Renderer/Renderer3D.hpp"
#include "Toaster/Renderer/Framebuffer.hpp"



// Toaster OpenGL API functions
#include "platform/OpenGl/OpenGLShader.hpp"

// Toaster Engine Input Header
#include "Toaster/Core/Input.hpp"

// Toaster Engine KeyCodes and MouseCodes Headers
#include "Toaster/Core/KeyCodes.hpp"
#include "Toaster/Core/MouseCodes.hpp"

// Toaster Engine OpenGl Test Layer Header
#include "platform/OpenGl/OpenGlTestLayer.h"

// Toaster Engine Log Wrapper Header
#include "Toaster/Core/Log.hpp"
