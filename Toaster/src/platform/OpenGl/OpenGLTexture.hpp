#pragma once
#include "Toaster/Renderer/Texture.hpp"

namespace tst
{
	class OpenGLTexture : public tst::Texture
	{
	public:
		virtual ~OpenGLTexture() override;

		OpenGLTexture(const std::string &filepath);

		virtual void bind() const override;
		virtual void unbind() const override;

	private:
		uint32_t m_textureId;
	};

}
