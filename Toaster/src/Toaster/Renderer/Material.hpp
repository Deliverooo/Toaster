#pragma once
#include "Toaster/Core/Log.hpp"

#include "Shader.hpp"

#include <glm/glm.hpp>

#include "Texture.hpp"


namespace tst
{
	struct MaterialProperties
	{
		glm::vec3 ambient { 0.2f, 0.2f, 0.2f };
		glm::vec3 diffuse { 0.8f, 0.8f, 0.8f };
		glm::vec3 specular{ 1.0f, 1.0f, 1.0f };
		glm::vec3 emissive{ 0.0f, 0.0f, 0.0f };

		float shininess			{ 48.0f };
		float opacity			{ 1.0f  };
		float indexOfRefraction { 1.55f };


		float metallic { 0.0f };
		float roughness{ 0.6f };
		float ao	   { 1.0f };
	};

	class TST_API Material
	{
	public:

		Material(const std::string &name = "Default");
		~Material() = default;

		const std::string& getName() const { return m_Name; }
		const MaterialProperties& getMaterialProperties() const { return m_MaterialProperties; }

		const RefPtr<Texture2D>& getDiffuseMap()  const { return m_DiffuseMap; }
		const RefPtr<Texture2D>& getSpecularMap() const { return m_SpecularMap; }
		const RefPtr<Texture2D>& getNormalMap()   const { return m_NormalMap; }
		const RefPtr<Texture2D>& getHeightMap()   const { return m_HeightMap; }

		void setName(const std::string& name) { m_Name = name; }
		void setMaterialProperties(const MaterialProperties& materialProperties) { m_MaterialProperties = materialProperties; }

		void setAmbient(const glm::vec3& ambient)	{ m_MaterialProperties.ambient = ambient;	  }
		void setDiffuse(const glm::vec3& diffuse)	{ m_MaterialProperties.diffuse = diffuse;	  }
		void setSpecular(const glm::vec3& specular) { m_MaterialProperties.specular = specular;	  }
		void setEmissive(const glm::vec3& emissive) { m_MaterialProperties.emissive = emissive;	  }
		void setShininess(float shininess)			{ m_MaterialProperties.shininess = shininess; }
		void setOpacity(float opacity)				{ m_MaterialProperties.opacity = opacity;	  }


		void setDiffuseMap(const RefPtr<Texture2D>& diffuseMap)  { m_DiffuseMap  = diffuseMap;  }
		void setSpecularMap(const RefPtr<Texture2D>& specularMap){ m_SpecularMap = specularMap; }
		void setNormalMap(const RefPtr<Texture2D>& normalMap)	 { m_NormalMap   = normalMap;   }
		void setHeightMap(const RefPtr<Texture2D>& heightMap)	 { m_HeightMap   = heightMap;	}

		bool hasTexture() const;
		void bind(const RefPtr<Shader>& shader) const;

		static RefPtr<Material> create(const std::string& name = "DefaultMat");
		static RefPtr<Material> createDefault();

	private:

		std::string m_Name;
		MaterialProperties m_MaterialProperties;

		RefPtr<Texture2D> m_DiffuseMap;
		RefPtr<Texture2D> m_SpecularMap;
		RefPtr<Texture2D> m_NormalMap;
		RefPtr<Texture2D> m_HeightMap;
	};

	class MaterialLibrary
	{
	public:
		MaterialLibrary() = default;
		~MaterialLibrary() = default;

		void addMaterial(const RefPtr<Material>& material);
		RefPtr<Material> getMaterial(const std::string& name) const;
		RefPtr<Material> getMaterial(uint32_t index) const;

		bool hasMaterial(const std::string& name) const;
		uint32_t getMaterialCount() const { return static_cast<uint32_t>(m_materials.size()); }

		void clear() { m_materials.clear(); }

		//bool loadFromMtlFile(const std::string& filepath);

	private:
		std::vector<RefPtr<Material>> m_materials;
		std::unordered_map<std::string, uint32_t> m_materialIndices;
	};
}
