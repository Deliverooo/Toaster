#include "tstpch.h"
#include "SceneSerializer.hpp"

#include <yaml-cpp/yaml.h>
#include "Entity.hpp"

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
		out << YAML::Key << "Entity" << YAML::Value << "Id";

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
	void SceneSerializer::deserialize(const std::string& filepath)
	{
		// Deserialization logic goes here
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