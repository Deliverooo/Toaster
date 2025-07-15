#pragma once

namespace tst
{
	class TST_API Texture
	{
	public:
		virtual ~Texture() {}

		static std::shared_ptr<Texture> create(const std::string& filepath);

		virtual void bind() const = 0;
		virtual void unbind() const = 0;
	};
}
