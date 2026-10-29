#include "toast_kernel/camera.hpp"
#include <cmath>

#undef near
#undef far

namespace toaster::knl
{
	Camera::Camera(float32 p_fov, float32 p_aspec_ratio, float32 p_z_near, float32 p_z_far) : m_fov(p_fov), m_aspect(p_aspec_ratio), m_zNear(p_z_near), m_zFar(p_z_far)
	{
		_calcProjection();
	}

	auto Camera::onUpdate(const InputContext *p_input_ctx, float32 p_dt) -> void
	{
		auto pos{getPosition()};

		auto front{getCameraForward()};
		auto right{getCameraRight()};

		math::Vector delta_position{XMVectorZero()};

		if (input_is_key_down(p_input_ctx, EKeyCode::eW))
			delta_position += front;
		if (input_is_key_down(p_input_ctx, EKeyCode::eS))
			delta_position -= front;
		if (input_is_key_down(p_input_ctx, EKeyCode::eA))
			delta_position -= right;
		if (input_is_key_down(p_input_ctx, EKeyCode::eD))
			delta_position += right;

		if (input_is_key_down(p_input_ctx, EKeyCode::eSpace))
			delta_position -= unitUpDir;
		if (input_is_key_down(p_input_ctx, EKeyCode::eLeftControl))
			delta_position += unitUpDir;

		m_speed = input_is_key_down(p_input_ctx, EKeyCode::eLeftShift) ? 30.0f : 10.0f;

		delta_position = math::XMVector3NormalizeSafe(delta_position) * p_dt * m_speed;

		pos += delta_position;

		XMStoreFloat3(&m_position, pos);

		const math::Float2 mouse_delta{input_get_mouse_delta_pos(p_input_ctx)};

		m_yaw   += p_dt * mouse_delta.x * 100.0f;
		m_pitch += p_dt * mouse_delta.y * 100.0f;

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

	auto Camera::getFrustumPlanes() const -> Array<math::Vector, 6u>
	{
		math::Matrix clip_matrix{XMMatrixTranspose(getViewMatrix() * getProjectionMatrix())};

		math::Vector left{clip_matrix.r[3u] + clip_matrix.r[0u]};
		math::Vector right{clip_matrix.r[3u] - clip_matrix.r[0u]};
		math::Vector bottom{clip_matrix.r[3u] + clip_matrix.r[1u]};
		math::Vector top{clip_matrix.r[3u] - clip_matrix.r[1u]};
		math::Vector near{clip_matrix.r[2u]};
		math::Vector far{clip_matrix.r[3u] - clip_matrix.r[2u]};

		Array<math::Vector, 6u> simd_planes{left, right, bottom, top, near, far};

		for (uint32 i{0u}; i < 6u; ++i)
			simd_planes[i] = XMPlaneNormalize(simd_planes[i]);

		return simd_planes;
	}

	auto Camera::_calcProjection() -> void
	{
		float32      cot_half_fov{1.0f / std::tanf(XMConvertToRadians(m_fov) / 2.0f)};
		math::Matrix proj{
			XMMatrixSet(cot_half_fov / m_aspect, 0.0f, 0.0f, 0.0f, 0.0f, cot_half_fov, 0.0f, 0.0f, 0.0f, 0.0f, (m_zNear + m_zFar) / (m_zFar - m_zNear),
						(-2.0f * m_zNear * m_zFar) / (m_zFar - m_zNear), 0.0f, 0.0f, 1.0f, 0.0f)
		};

		// XMMATRIX proj{XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), m_aspect, m_zNear, m_zFar)};

		XMStoreFloat4x4(&m_projectionMatrix, XMMatrixTranspose(proj));
	}
}
