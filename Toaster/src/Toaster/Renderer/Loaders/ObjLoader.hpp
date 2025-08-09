#pragma once
#include "Toaster/Renderer/Mesh.hpp"

namespace tst
{
	class ObjLoader : public MeshLoader
	{
	public:
		virtual ~ObjLoader() override = default;

		virtual bool load(const std::string& filepath,
			std::vector<MeshVertex>& vertices,
			std::vector<uint32_t>& indices,
			std::vector<SubMesh>& submeshes,
			std::vector<MaterialID>& material_ids) override;


		virtual std::vector<std::string> getSupportedExtensions() const override
		{
			return { ".obj" };
		}
	};
}
