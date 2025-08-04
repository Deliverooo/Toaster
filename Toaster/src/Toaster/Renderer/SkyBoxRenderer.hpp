#pragma once
#include "Texture.hpp"
#include "Toaster/Renderer/Camera.hpp"

namespace tst {

	class SkyBoxRenderer
	{
	public:

		static void init();
		static void terminate() noexcept;

		static void render(const Camera& camera, const glm::mat4& view);

		static void setSkyboxTexture(const RefPtr<Texture3D>& texture);

	private:
	};
}