#pragma once

#include "input.hpp"


#include "toast_math/math_matrix.hpp"
using namespace DirectX;

namespace toaster
{
	constexpr XMVECTORF32 unitForwardDir{.f{0.0f, 0.0f, -1.0f, 0.0f}};
	constexpr XMVECTORF32 unitRightDir{.f{1.0f, 0.0f, 0.0f, 0.0f}};
	constexpr XMVECTORF32 unitUpDir{.f{0.0f, -1.0f, 0.0f, 0.0f}};

	struct TST_KERNEL_API CameraCB
	{
		XMFLOAT4X4 view;
		XMFLOAT4X4 proj;
	};

	class TST_KERNEL_API Camera
	{
	public:
		Camera() = default;
		Camera(float32 p_fov, float32 p_aspec_ratio, float32 p_z_near = 0.1f, float32 p_z_far = 1000.0f);

		auto onUpdate(const InputContext &p_input_ctx, float32 p_dt) -> void;
		auto onResize(float32 p_aspect) -> void;

		auto XM_CALLCONV getViewMatrix() const -> XMMATRIX { return XMMatrixLookToLH(XMLoadFloat3(&m_position), getCameraForward(), unitUpDir); }
		auto XM_CALLCONV getProjectionMatrix() const -> XMMATRIX { return XMLoadFloat4x4(&m_projectionMatrix); }

		auto XM_CALLCONV getRotationMatrix() const -> XMMATRIX
		{
			return XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_pitch), -XMConvertToRadians(m_yaw), 0.0f);
		}

		// Rotates the unit direction vectors by the camera's rotation matrix :)
		auto XM_CALLCONV getCameraForward() const -> XMVECTOR { return XMVector3Transform(unitForwardDir, getRotationMatrix()); }
		auto XM_CALLCONV getCameraRight() const -> XMVECTOR { return XMVector3Transform(unitRightDir, getRotationMatrix()); }
		auto XM_CALLCONV getCameraUp() const -> XMVECTOR { return XMVector3Transform(unitUpDir, getRotationMatrix()); }

		auto XM_CALLCONV getPosition() const -> XMVECTOR { return XMLoadFloat3(&m_position); }

		auto populateConstantBuffer(CameraCB &p_out_cb) const -> void;

	private:
		auto _calcProjection() -> void;

		XMFLOAT4X4 m_projectionMatrix;

		XMFLOAT3 m_position{0.0f, 2.0f, -2.0f};

		float32 m_yaw{0.0f};
		float32 m_pitch{0.0f};

		float32 m_fov{90.0f};
		float32 m_aspect{1920.0f / 1080.0f};

		float32 m_zNear{0.1f};
		float32 m_zFar{1000.0f};

		float32 m_speed{10.0f};
	};
}
