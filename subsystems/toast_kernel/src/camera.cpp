#include "toast_kernel/camera.hpp"

#include <print>
#include <toast_math/math_vector.hpp>

namespace toaster
{
	Camera::Camera(float32 p_fov, float32 p_aspec_ratio, float32 p_z_near, float32 p_z_far) : m_fov(p_fov), m_aspect(p_aspec_ratio), m_zNear(p_z_near), m_zFar(p_z_far)
	{
		_calcProjection();
	}

	auto Camera::onUpdate(const InputContext &p_input_ctx, float32 p_dt) -> void
	{
		auto pos{getPosition()};

		auto front{getCameraForward()};
		auto right{getCameraRight()};

		XMVECTOR delta_position{XMVectorZero()};

		if (p_input_ctx.isKeyDown(EKeyCode::eW))
			delta_position += front;
		if (p_input_ctx.isKeyDown(EKeyCode::eS))
			delta_position -= front;
		if (p_input_ctx.isKeyDown(EKeyCode::eA))
			delta_position -= right;
		if (p_input_ctx.isKeyDown(EKeyCode::eD))
			delta_position += right;

		if (p_input_ctx.isKeyDown(EKeyCode::eSpace))
			delta_position -= unitUpDir;
		if (p_input_ctx.isKeyDown(EKeyCode::eLeftControl))
			delta_position += unitUpDir;

		m_speed = p_input_ctx.isKeyDown(EKeyCode::eLeftShift) ? 30.0f : 10.0f;

		delta_position = XMVector3NormalizeSafe(delta_position) * p_dt * m_speed;

		pos += delta_position;

		XMStoreFloat3(&m_position, pos);

		float32 mdx{p_input_ctx.getMouseDx()};
		float32 mdy{p_input_ctx.getMouseDy()};

		m_yaw   += p_dt * mdx * 100.0f;
		m_pitch += p_dt * mdy * 100.0f;

		if (m_pitch > 89.0f)
			m_pitch = 89.0f;
		if (m_pitch < -89.0f)
			m_pitch = -89.0f;
	}

	auto Camera::onResize(float32 p_aspect) -> void
	{
		m_aspect = p_aspect;
		_calcProjection();
	}

	auto Camera::getFrustumPlanes() const -> std::array<XMVECTOR, 6u>
	{
		XMMATRIX clip_matrix{XMMatrixTranspose(getViewMatrix() * getProjectionMatrix())};

		XMVECTOR left{clip_matrix.r[3u] + clip_matrix.r[0u]};
		XMVECTOR right{clip_matrix.r[3u] - clip_matrix.r[0u]};
		XMVECTOR bottom{clip_matrix.r[3u] + clip_matrix.r[1u]};
		XMVECTOR top{clip_matrix.r[3u] - clip_matrix.r[1u]};
		XMVECTOR near{clip_matrix.r[2u]};
		XMVECTOR far{clip_matrix.r[3u] - clip_matrix.r[2u]};

		std::array simd_planes{left, right, bottom, top, near, far};

		for (uint32 i{0u}; i < 6u; ++i)
			simd_planes[i] = XMPlaneNormalize(simd_planes[i]);

		return simd_planes;
	}

	auto Camera::_calcProjection() -> void
	{
		float32 cot_half_fov{1.0f / std::tanf(XMConvertToRadians(m_fov) / 2.0f)};
		XMMATRIX proj{
		XMMatrixSet(cot_half_fov / m_aspect, 0.0f, 0.0f, 0.0f, 0.0f, cot_half_fov, 0.0f, 0.0f, 0.0f, 0.0f, (m_zNear + m_zFar) / (m_zFar - m_zNear),
		(-2.0f * m_zNear * m_zFar) / (m_zFar - m_zNear), 0.0f, 0.0f, 1.0f, 0.0f)
		};

		// XMMATRIX proj{XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), m_aspect, m_zNear, m_zFar)};

		XMStoreFloat4x4(&m_projectionMatrix, XMMatrixTranspose(proj));
	}
}
