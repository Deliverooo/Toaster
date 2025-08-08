#pragma once
#include <variant>

#include "Toaster/Core/Log.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>
#include "Texture.hpp"


namespace tst
{
	struct MaterialProperties
	{
		glm::vec3 ambient { 0.0f, 0.0f, 0.0f };
		glm::vec3 diffuse { 0.8f, 0.8f, 0.8f };
		glm::vec3 specular{ 1.0f, 1.0f, 1.0f };
		glm::vec3 emissive{ 0.0f, 0.0f, 0.0f };

		float shininess			{ 64.0f };
		float opacity			{ 1.0f  };
		float indexOfRefraction { 1.55f };
		float metallic { 0.0f };
		float roughness{ 0.6f };
		float ao	   { 1.0f };

		bool backfaceCulling{ false };
	};

	using MaterialPropertyType = std::variant<glm::vec3, glm::vec4, float, int, bool, glm::mat3, glm::mat4>;


	struct RenderState
	{
		bool depthTest{ true };
		bool backfaceCulling{ false };
		bool blending{ true };

		enum class BlendMode
		{
			None,
			AlphaBlend,
			Additive,
			Multiply
		} blendMode{ BlendMode::None };

		bool wireframe{ false };
	};
	struct TextureBinding
	{
		RefPtr<Texture2D> texture;
		std::string uniformName;
		bool hasTexture{ false };
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

		void setAmbient(const glm::vec3& ambient)	  { m_MaterialProperties.ambient = ambient;	  }
		void setDiffuse(const glm::vec3& diffuse)	  { m_MaterialProperties.diffuse = diffuse;	  }
		void setSpecular(const glm::vec3& specular)   { m_MaterialProperties.specular = specular;	  }
		void setEmissive(const glm::vec3& emissive)   { m_MaterialProperties.emissive = emissive;	  }
		void setShininess(float shininess)			  { m_MaterialProperties.shininess = shininess; }
		void setOpacity(float opacity)				  { m_MaterialProperties.opacity = opacity;	  }
		void setMetallic(float metallic)			  { m_MaterialProperties.metallic = metallic; }
		void setRoughness(float roughness)			  { m_MaterialProperties.roughness = roughness; }



		template <typename T>
		void setProperty(const std::string& name, const T& value)
		{
			static_assert(std::is_constructible_v<MaterialPropertyType, T> && "Material Property type is not supported");
			m_CustomProperties[name] = value;
			makeDirty();
		}

		template <typename T>
		T getProperty(const std::string& name) const
		{

			auto it = m_CustomProperties.find({ name });
			if (it != m_CustomProperties.end())
			{
				if (auto* val = std::get_if<T>(&it->second)) {
					return *val;
				}
			}
			TST_CORE_ERROR("Material property '{0}' not found or type mismatch", name);
			return T{};
		}

		const RenderState& getRenderState() const { return m_RenderState; }
		void setRenderState(const RenderState& renderState) { m_RenderState = renderState; makeDirty(); }

		void setBackfaceCulling(bool backfaceCulling) { m_RenderState.backfaceCulling = backfaceCulling; makeDirty(); }


		void makeDirty() { m_IsDirty = true; }

		void setDiffuseMap(const RefPtr<Texture2D>& diffuseMap)  { m_DiffuseMap  = diffuseMap;  }
		void setSpecularMap(const RefPtr<Texture2D>& specularMap){ m_SpecularMap = specularMap; }
		void setNormalMap(const RefPtr<Texture2D>& normalMap)	 { m_NormalMap   = normalMap;   }
		void setHeightMap(const RefPtr<Texture2D>& heightMap)	 { m_HeightMap   = heightMap;	}

		bool hasTexture() const;
		void bind(const RefPtr<Shader>& shader) const;
		void unbind() const;

		static RefPtr<Material> create(const std::string& name = "DefaultMat");
		static RefPtr<Material> createDefault();

		static const std::string s_baseUniformName;

	private:

		struct MaterialUniformUploader;

		void bindTextures() const;
		void uploadUniforms(const RefPtr<Shader> &shader) const;


		mutable bool m_IsDirty{ true };
		mutable std::weak_ptr<Shader> m_lastBoundShader;

		std::string m_Name;
		MaterialProperties m_MaterialProperties;

		std::unordered_map<std::string, MaterialPropertyType> m_CustomProperties;


		RenderState m_RenderState;

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

	private:
		std::vector<RefPtr<Material>> m_materials;
		std::unordered_map<std::string, uint32_t> m_materialIndices;
	};
}
