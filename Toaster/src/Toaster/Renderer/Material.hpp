#pragma once
#include <variant>

#include "Toaster/Core/Log.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>

#include "Texture.hpp"
#include "Toaster/Asset/Asset.hpp"

// Forward declaration to avoid circular dependency
namespace tst { using MaterialID = uint32_t; }

namespace tst
{

	// TODO - Remove ts

	//  -=[Legacy]=-
	//struct MaterialProperties
	//{
	//	glm::vec3 ambient{ 0.0f, 0.0f, 0.0f };
	//	glm::vec3 diffuse{ 0.8f, 0.8f, 0.8f };
	//	glm::vec3 specular{ 1.0f, 1.0f, 1.0f };
	//	glm::vec3 emissive{ 0.0f, 0.0f, 0.0f };

	//	float shininess{ 64.0f };
	//	float opacity{ 1.0f };
	//	float indexOfRefraction{ 1.55f };
	//	float metallic{ 0.0f };
	//	float roughness{ 0.6f };
	//	float ao{ 1.0f };

	//	bool backfaceCulling{ false };
	//};
	//  -=[Legacy]=-

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

		Material(const std::string& name = "Default");
		virtual ~Material() = default;

		const std::string& getName() const { return m_Name; }

		//  -=[Legacy]=-
		//const MaterialProperties& getMaterialProperties() const { return m_MaterialProperties; }
		//  -=[Legacy]=-

		virtual const RefPtr<Texture2D>& getDiffuseMap()  const { return m_DiffuseMap;	}
		virtual const RefPtr<Texture2D>& getSpecularMap() const { return m_SpecularMap; }
		virtual const RefPtr<Texture2D>& getNormalMap()   const { return m_NormalMap;	}
		virtual const RefPtr<Texture2D>& getHeightMap()   const { return m_HeightMap;	}

		void setName(const std::string& name) { m_Name = name; }

		//  -=[Legacy]=-
		//void setMaterialProperties(const MaterialProperties& materialProperties) { m_MaterialProperties = materialProperties; }

		//void setAmbient(const glm::vec3& ambient)	{ m_MaterialProperties.ambient = ambient;		}
		//void setDiffuse(const glm::vec3& diffuse)	{ m_MaterialProperties.diffuse = diffuse;		}
		//void setSpecular(const glm::vec3& specular) { m_MaterialProperties.specular = specular;		}
		//void setEmissive(const glm::vec3& emissive) { m_MaterialProperties.emissive = emissive;		}
		//void setShininess(float shininess)			{ m_MaterialProperties.shininess = shininess;	}
		//void setOpacity(float opacity)				{ m_MaterialProperties.opacity = opacity;		}
		//void setMetallic(float metallic)			{ m_MaterialProperties.metallic = metallic;		}
		//void setRoughness(float roughness)			{ m_MaterialProperties.roughness = roughness;	}
		//  -=[Legacy]=-

		virtual void setProperty_int(const std::string& property_name,		int		  value);
		virtual void setProperty_bool(const std::string& property_name,		bool	  value);
		virtual void setProperty_float(const std::string& property_name,	float	  value);
		virtual void setProperty_vec3(const std::string& property_name,		glm::vec3 value);
		virtual void setProperty_vec4(const std::string& property_name,		glm::vec4 value);
		virtual void setProperty_mat3(const std::string& property_name,		glm::mat3 value);
		virtual void setProperty_mat4(const std::string& property_name,		glm::mat4 value);

		virtual int		  getProperty_int(const std::string& property_name) const;
		virtual bool	  getProperty_bool(const std::string& property_name) const;
		virtual float	  getProperty_float(const std::string& property_name) const;
		virtual glm::vec3 getProperty_vec3(const std::string& property_name) const;
		virtual glm::vec4 getProperty_vec4(const std::string& property_name) const;
		virtual glm::mat3 getProperty_mat3(const std::string& property_name) const;
		virtual glm::mat4 getProperty_mat4(const std::string& property_name) const;

		template <typename T>
		void setProperty(const std::string& name, const T& value)
		{
			static_assert(std::is_constructible_v<MaterialPropertyType, T> && "Material Property type is not supported");
			setPropertyImpl(name, value);
		}

		template <typename T>
		T getProperty(const std::string& name) const
		{
			return getPropertyImpl<T>(name);
		}

		virtual bool hasProperty(const std::string& property_name) const;


		const RenderState& getRenderState() const { return m_RenderState; }
		void setRenderState(const RenderState& renderState) { m_RenderState = renderState; makeDirty(); }
		void setBackfaceCulling(bool backfaceCulling) { m_RenderState.backfaceCulling = backfaceCulling; makeDirty(); }


		void setDiffuseMap(const RefPtr<Texture2D>& diffuseMap) { m_DiffuseMap = diffuseMap; }
		void setSpecularMap(const RefPtr<Texture2D>& specularMap) { m_SpecularMap = specularMap; }
		void setNormalMap(const RefPtr<Texture2D>& normalMap) { m_NormalMap = normalMap; }
		void setHeightMap(const RefPtr<Texture2D>& heightMap) { m_HeightMap = heightMap; }

		virtual bool hasTexture() const;
		virtual void bind(const RefPtr<Shader>& shader) const;
		virtual void unbind() const;

		virtual void makeDirty() { m_IsDirty = true;  }
		void markClean() const	 { m_IsDirty = false; }
		bool isDirty() const	 { return m_IsDirty;  }

		static RefPtr<Material> create(const std::string& name = "DefaultMat");
		static RefPtr<Material> createDefault();

		static const std::string s_baseUniformName;

		virtual void uploadUniforms(const RefPtr<Shader>& shader) const;
	protected:

		struct MaterialUniformUploader;

		virtual void bindTextures() const;

		mutable bool m_IsDirty{ true };
		mutable std::weak_ptr<Shader> m_lastBoundShader;

		std::string m_Name;
		//MaterialProperties m_MaterialProperties;
		std::unordered_map<std::string, MaterialPropertyType> m_MaterialProperties;
		RenderState m_RenderState;

		RefPtr<Texture2D> m_DiffuseMap;
		RefPtr<Texture2D> m_SpecularMap;
		RefPtr<Texture2D> m_NormalMap;
		RefPtr<Texture2D> m_HeightMap;

		template<typename T>
		void setPropertyImpl(const std::string& property_name, const T& value);

		template<typename T>
		T getPropertyImpl(const std::string& property_name) const;
	};





	class MaterialInstance : public Material
	{
	public:

		MaterialInstance(MaterialID base_material_id, const std::string& instance_name);
		virtual ~MaterialInstance() override = default;

		virtual void setProperty_int(const std::string& property_name, int		  value) override;
		virtual void setProperty_bool(const std::string& property_name, bool	  value) override;
		virtual void setProperty_float(const std::string& property_name, float	  value) override;
		virtual void setProperty_vec3(const std::string& property_name, glm::vec3 value) override;
		virtual void setProperty_vec4(const std::string& property_name, glm::vec4 value) override;
		virtual void setProperty_mat3(const std::string& property_name, glm::mat3 value) override;
		virtual void setProperty_mat4(const std::string& property_name, glm::mat4 value) override;

		virtual int		  getProperty_int(const std::string& property_name) const override;
		virtual bool	  getProperty_bool(const std::string& property_name) const override;
		virtual float	  getProperty_float(const std::string& property_name) const override;
		virtual glm::vec3 getProperty_vec3(const std::string& property_name) const override;
		virtual glm::vec4 getProperty_vec4(const std::string& property_name) const override;
		virtual glm::mat3 getProperty_mat3(const std::string& property_name) const override;
		virtual glm::mat4 getProperty_mat4(const std::string& property_name) const override;

		virtual bool hasProperty(const std::string& property_name) const override;

		virtual const RefPtr<Texture2D>& getDiffuseMap()	const override;
		virtual const RefPtr<Texture2D>& getSpecularMap()   const override;
		virtual const RefPtr<Texture2D>& getNormalMap()		const override;
		virtual const RefPtr<Texture2D>& getHeightMap()		const override;


		virtual bool hasTexture() const override;

		virtual void bind(const RefPtr<Shader>& shader) const override;
		virtual void unbind() const override;

		bool hasPropertyOverride(const std::string& property_name) const;
		void resetProperty(const std::string& property_name);

		// texture_type - e.g. Diffuse, Specular, Normal ...
		void resetTextureOverride(const std::string& texture_type);

		const std::unordered_map<std::string, MaterialPropertyType> &getPropertyOverrides() const
		{
			return m_PropertyOverrides;
		}


		MaterialID getBaseMaterialID() const { return m_BaseMaterialID; }
		RefPtr<Material> getBaseMaterial() const;

		void setDiffuseMapOverride(const RefPtr<Texture2D>& diffuse_map);
		void setSpecularMapOverride(const RefPtr<Texture2D>& specular_map);
		void setNormalMapOverride(const RefPtr<Texture2D>& normal_map);
		void setHeightMapOverride(const RefPtr<Texture2D>& height_map);

		static RefPtr<MaterialInstance> create(MaterialID base_material_id, const std::string& instance_name);

	protected:

		virtual void uploadUniforms(const RefPtr<Shader>& shader) const override;
		virtual void bindTextures() const override;

		MaterialID m_BaseMaterialID;
		std::unordered_map<std::string, MaterialPropertyType> m_PropertyOverrides;

		RefPtr<Texture2D> m_DiffuseMapOverride;
		RefPtr<Texture2D> m_SpecularMapOverride;
		RefPtr<Texture2D> m_NormalMapOverride;
		RefPtr<Texture2D> m_HeightMapOverride;
	};


	// Material Set property template specializations

	template<>
	inline void Material::setPropertyImpl<int>(const std::string& property_name, const int& value)
	{
		setProperty_int(property_name, value);
	}
	template<>
	inline void Material::setPropertyImpl<bool>(const std::string& property_name, const bool& value)
	{
		setProperty_bool(property_name, value);
	}
	template<>
	inline void Material::setPropertyImpl<float>(const std::string& property_name, const float& value)
	{
		setProperty_float(property_name, value);
	}
	template<>
	inline void Material::setPropertyImpl<glm::vec3>(const std::string& property_name, const glm::vec3& value)
	{
		setProperty_vec3(property_name, value);
	}
	template<>
	inline void Material::setPropertyImpl<glm::vec4>(const std::string& property_name, const glm::vec4& value)
	{
		setProperty_vec4(property_name, value);
	}
	template<>
	inline void Material::setPropertyImpl<glm::mat3>(const std::string& property_name, const glm::mat3& value)
	{
		setProperty_mat3(property_name, value);
	}
	template<>
	inline void Material::setPropertyImpl<glm::mat4>(const std::string& property_name, const glm::mat4& value)
	{
		setProperty_mat4(property_name, value);
	}

	// Material get property template specializations
	// Since C++ doesn't support templated virtual functions,
	// I have opted to use type erasure to handle this

	template<>
	inline int Material::getPropertyImpl<int>(const std::string& property_name) const
	{
		return getProperty_int(property_name);
	}
	template<>
	inline bool Material::getPropertyImpl<bool>(const std::string& property_name) const
	{
		return getProperty_bool(property_name);
	}
	template<>
	inline float Material::getPropertyImpl<float>(const std::string& property_name) const
	{
		return getProperty_float(property_name);
	}
	template<>
	inline glm::vec3 Material::getPropertyImpl<glm::vec3>(const std::string& property_name) const
	{
		return getProperty_vec3(property_name);
	}
	template<>
	inline glm::vec4 Material::getPropertyImpl<glm::vec4>(const std::string& property_name) const
	{
		return getProperty_vec4(property_name);
	}
	template<>
	inline glm::mat3 Material::getPropertyImpl<glm::mat3>(const std::string& property_name) const
	{
		return getProperty_mat3(property_name);
	}
	template<>
	inline glm::mat4 Material::getPropertyImpl<glm::mat4>(const std::string& property_name) const
	{
		return getProperty_mat4(property_name);
	}

	// Legacy material library system
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
