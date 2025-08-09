#pragma once
#include <any>
#include <variant>

namespace tst
{
	enum class AssetType : uint8_t
	{
		None = 0,
		Texture2D,
		Texture3D,
		Mesh,
		Material,
		Shader,
		Audio,
		Scene,
		Script,
	};

	class TST_API Asset
	{
	public:

		Asset() = default;
		virtual ~Asset() = default;

		std::string name;
		std::string filepath;



		virtual AssetType getType() const = 0;
		virtual bool isLoaded() const = 0;
		virtual void reload() {}

		//bool operator==(const Asset& other) const { return handle == other.handle; }


	protected:

		AssetType m_AssetType{ AssetType::None };
	};


	class TST_API AssetSerializer
	{
	public:

		
	private:
		
	};
}