#pragma once

#include "input.hpp"
#include "toast_kernel.hpp"

namespace toaster::knl
{
	constexpr XMVECTORF32 unitForwardDir{.f{0.0f, 0.0f, -1.0f, 0.0f}};
	constexpr XMVECTORF32 unitRightDir{.f{1.0f, 0.0f, 0.0f, 0.0f}};
	constexpr XMVECTORF32 unitUpDir{.f{0.0f, -1.0f, 0.0f, 0.0f}};

	struct TST_KERNEL_API CameraCB
	{
		math::Float4x4 view;
		math::Float4x4 proj;
		math::Float4x4 invProj;

		math::Float4 position;
		math::Float4 frustumPlanes[6u];
	};

	class TST_KERNEL_API Camera
	{
	public:
		Camera() = default;
		Camera(float32 p_fov, float32 p_aspec_ratio, float32 p_z_near = 0.1f, float32 p_z_far = 1000.0f);

		auto onUpdate(const InputContext *p_input_ctx, float32 p_dt) -> void;
		auto onResize(float32 p_aspect) -> void;

		auto TST_MATH_CALLCONV getViewMatrix() const -> math::Matrix { return XMMatrixLookToLH(XMLoadFloat3(&m_position), getCameraForward(), unitUpDir); }
		auto TST_MATH_CALLCONV getProjectionMatrix() const -> math::Matrix { return XMLoadFloat4x4(&m_projectionMatrix); }

		auto TST_MATH_CALLCONV getRotationMatrix() const -> math::Matrix
		{
			return XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_pitch), -XMConvertToRadians(m_yaw), 0.0f);
		}

		// Rotates the unit direction vectors by the camera's rotation matrix :)
		auto TST_MATH_CALLCONV getCameraForward() const -> math::Vector { return XMVector3Transform(unitForwardDir, getRotationMatrix()); }
		auto TST_MATH_CALLCONV getCameraRight() const -> math::Vector { return XMVector3Transform(unitRightDir, getRotationMatrix()); }
		auto TST_MATH_CALLCONV getCameraUp() const -> math::Vector { return XMVector3Transform(unitUpDir, getRotationMatrix()); }

		auto TST_MATH_CALLCONV getPosition() const -> math::Vector { return XMLoadFloat3(&m_position); }

		[[nodiscard]] auto TST_MATH_CALLCONV getFrustumPlanes() const -> Array<math::Vector, 6u>;

	private:
		auto _calcProjection() -> void;

		math::Float4x4 m_projectionMatrix;

		math::Float3 m_position{0.0f, 2.0f, -2.0f};

		float32 m_yaw{0.0f};
		float32 m_pitch{0.0f};

		float32 m_fov{90.0f};
		float32 m_aspect{1920.0f / 1080.0f};

		float32 m_zNear{0.1f};
		float32 m_zFar{1000.0f};

		float32 m_speed{10.0f};
	};
}
