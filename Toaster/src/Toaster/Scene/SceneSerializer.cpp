#include "tstpch.h"
#include "SceneSerializer.hpp"

#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/convert.h>
#include <yaml-cpp/emitter.h>
#include "Entity.hpp"

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};


}

namespace tst
{

	SceneSerializer::SceneSerializer(const RefPtr<Scene>& scene)
		: m_Scene(scene)
	{

	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	static void serializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << "12241557773261097666";

		if (entity.hasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent" << YAML::BeginMap;
			auto& tag = entity.getComponent<TagComponent>();	
			out << YAML::Key << "Tag" << YAML::Value << tag.name;
			out << YAML::EndMap;
		}
		if (entity.hasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent" << YAML::BeginMap;
			auto& spriteRenderer = entity.getComponent<SpriteRendererComponent>();
			out << YAML::Key << "Colour" << YAML::Value << spriteRenderer.colour;
			std::string texturePath = "NULL";
			if (spriteRenderer.texture)
			{
				texturePath = spriteRenderer.texture->getPath().value();
			}
			out << YAML::Key << "Texture" << YAML::Value << texturePath;
			out << YAML::EndMap;
		}
		if (entity.hasComponent<MeshRendererComponent>())
		{
			out << YAML::Key << "MeshRendererComponent" << YAML::BeginMap;
			auto& meshRenderer = entity.getComponent<MeshRendererComponent>();
			out << YAML::Key << "Colour" << YAML::Value << meshRenderer.colour;
			out << YAML::Key << "Filepath" << YAML::Value << meshRenderer.mesh->getFilepath();
			out << YAML::EndMap;
		}
		if (entity.hasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent" << YAML::BeginMap;
			auto& transform = entity.getComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << transform.translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.scale;
			out << YAML::EndMap;
		}
		if (entity.hasComponent<LightComponent>())
		{
			out << YAML::Key << "LightComponent" << YAML::BeginMap;
			auto& light = entity.getComponent<LightComponent>();
			out << YAML::Key << "Type" << YAML::Value << static_cast<int>(light.light.type);
			out << YAML::Key << "Colour" << YAML::Value << light.light.colour;
			out << YAML::Key << "Intensity" << YAML::Value << light.light.intensity;
			out << YAML::Key << "Constant" << YAML::Value << light.light.constant;
			out << YAML::Key << "Linear" << YAML::Value << light.light.linear;
			out << YAML::Key << "Quadratic" << YAML::Value << light.light.quadratic;
			out << YAML::Key << "InnerCone" << YAML::Value << light.light.innerCone;
			out << YAML::Key << "OuterOffset" << YAML::Value << light.light.outerOffset;
			out << YAML::Key << "Enabled" << YAML::Value << light.enabled;
			out << YAML::EndMap;
		}

		if (entity.hasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap;

			auto& cameraComponent = entity.getComponent<CameraComponent>();
			auto& camera = cameraComponent.camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "ProjectionType" << YAML::Value << static_cast<int>(camera.getProjectionType());
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.getPerspectiveFov();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.getPerspectiveNear();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.getPerspectiveFar();
			out << YAML::Key << "OrthoSize" << YAML::Value << camera.getOrthoSize();
			out << YAML::Key << "OrthoNear" << YAML::Value << camera.getOrthoNear();
			out << YAML::Key << "OrthoFar" << YAML::Value << camera.getPerspectiveNear();
			out << YAML::EndMap;

			out << YAML::Key << "Active" << YAML::Value << cameraComponent.active;
			out << YAML::Key << "FixedAspect" << YAML::Value << cameraComponent.fixedAspect;

			out << YAML::EndMap;
		}

		out << YAML::EndMap;
	}

	void SceneSerializer::serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Unnamed Scene";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		auto sceneView = m_Scene->m_registry.view<entt::entity>();
		for (auto entity : sceneView) {

			Entity e = { entity, m_Scene.get() };
			if (!e) { continue; }

			serializeEntity(out, e);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
		std::ofstream file(filepath);
		file << out.c_str();
	}
	bool SceneSerializer::deserialize(const std::string& filepath)
	{
		std::ifstream file(filepath);
		std::stringstream strStream;
		strStream << file.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
		{
			TST_CORE_ERROR("Invalid scene file format: {0}", filepath);
			return false;
		}
		std::string sceneName = data["Scene"].as<std::string>();
		TST_CORE_INFO("Deserializing scene {0}", sceneName);

		auto entities = data["Entities"];

		if (entities)
		{
			for (const auto& entity : entities)
			{
				uint64_t uuId = entity["Entity"].as<uint64_t>();

				std::string name;
				auto& tagComponent = entity["TagComponent"];
				if (tagComponent)
				{
					name = tagComponent["Tag"].as<std::string>();
				}

				Entity deserializedEntity = m_Scene->createEntity(name);

				auto transformComp = entity["TransformComponent"];
				if (transformComp)
				{
					auto& tc = deserializedEntity.getComponent<TransformComponent>();
					tc.translation = transformComp["Translation"].as<glm::vec3>();
					tc.rotation = transformComp["Rotation"].as<glm::vec3>();
					tc.scale = transformComp["Scale"].as<glm::vec3>();
				}

				auto spriteRendererComp = entity["SpriteRendererComponent"];
				if (spriteRendererComp)
				{
					auto& src = deserializedEntity.addComponent<SpriteRendererComponent>();
					src.colour = spriteRendererComp["Colour"].as<glm::vec4>();
					std::string texturePath = spriteRendererComp["Texture"].as<std::string>();
					if (texturePath != "NULL")
					{
						src.texture = Texture2D::create(texturePath);
					}
				}

				auto meshRendererComp = entity["MeshRendererComponent"];
				if (meshRendererComp)
				{
					auto& mrc = deserializedEntity.addComponent<MeshRendererComponent>();
					mrc.colour = meshRendererComp["Colour"].as<glm::vec4>();
					std::string meshPath = meshRendererComp["Filepath"].as<std::string>();
					if (!meshPath.empty() || meshPath != "Null")
					{
						mrc.mesh = Mesh::create(meshPath);
					}
				}

				auto lightComp = entity["LightComponent"];
				if (lightComp)
				{
					auto& lc = deserializedEntity.addComponent<LightComponent>();
					lc.light.type = static_cast<Light::Type>(lightComp["Type"].as<int>());
					lc.light.colour = lightComp["Colour"].as<glm::vec3>();
					lc.light.intensity = lightComp["Intensity"].as<float>();
					lc.light.constant = lightComp["Constant"].as<float>();
					lc.light.linear = lightComp["Linear"].as<float>();
					lc.light.quadratic = lightComp["Quadratic"].as<float>();
					lc.light.innerCone = lightComp["InnerCone"].as<float>();
					lc.light.outerOffset = lightComp["OuterOffset"].as<float>();
					lc.enabled = lightComp["Enabled"].as<bool>();

				}

				auto cameraComp = entity["CameraComponent"];
				if (cameraComp)
				{
					auto& cc = deserializedEntity.addComponent<CameraComponent>();
					auto& camera = cc.camera;
					auto cameraNode = cameraComp["Camera"];
					camera.setProjectionType(static_cast<SceneCamera::ProjectionType>(cameraNode["ProjectionType"].as<int>()));
					camera.setPerspectiveFov(cameraNode["PerspectiveFOV"].as<float>());
					camera.setPerspectiveNear(cameraNode["PerspectiveNear"].as<float>());
					camera.setPerspectiveFar(cameraNode["PerspectiveFar"].as<float>());
					camera.setOrthoSize(cameraNode["OrthoSize"].as<float>());
					camera.setOrthoNear(cameraNode["OrthoNear"].as<float>());
					camera.setOrthoFar(cameraNode["OrthoFar"].as<float>());
					cc.active = cameraComp["Active"].as<bool>();
					cc.fixedAspect = cameraComp["FixedAspect"].as<bool>();
				}

			}
		}
		return true;
	}
	void SceneSerializer::serializeRuntime(const std::string& filepath)
	{
		// Runtime serialization logic goes here
	}
	void SceneSerializer::deserializeRuntime(const std::string& filepath)
	{
		// Runtime deserialization logic goes here
	}
}