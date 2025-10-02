#include <Engine/World/Entity.h>

namespace Spike {

	Entity::Entity(World* world) 
		: m_World(world),
		m_Parent(nullptr), 
		m_Position(0.f, 0.f, 0.f), 
		m_Rotation(0.f, 0.f, 0.f),
		m_Scale(1.f, 1.f, 1.f) 
	{
		m_Handle = (EntityHandle)world->m_Registry.create();
		UpdateWorldTransform();
	}

	Entity::~Entity() {
		m_World->m_Registry.destroy(m_Handle);
	}

	void Entity::UpdateWorldTransform() {

		Mat4x4 T = glm::translate(Mat4x4(1.0f), m_Position);

		Quaternion pitchRotation = glm::angleAxis(m_Rotation.x, Vec3{ 1.f, 0.f, 0.f });
		Quaternion yawRotation = glm::angleAxis(m_Rotation.y, Vec3{ 0.f, 1.f, 0.f });
		Quaternion rollRotation = glm::angleAxis(m_Rotation.z, Vec3{ 0.f, 0.f, 1.f });

		Mat4x4 R = glm::toMat4(yawRotation * pitchRotation * rollRotation);
		Mat4x4 S = glm::scale(glm::mat4(1.0f), m_Scale);

		// Final model matrix
		m_WorldTransform = T * R * S;
	}

	void Entity::SetScale(const Vec3& scale) {

		m_Scale = scale;
		DispatchTransformCallBack();
	}

	void Entity::SetPosition(const Vec3& pos) {

		m_Position = pos;
		DispatchTransformCallBack();
	}

	void Entity::SetRotation(const Vec3& rot) {

		m_Rotation = rot;
		DispatchTransformCallBack();
	}

	uint8_t Entity::AddTransformCallBack(TransformCallBack&& func) {
		uint8_t id = (uint8_t)m_TransformCallbacks.size();

		m_TransformCallbacks.push_back(std::move(func));
		return id;
	}

	void Entity::RemoveTransformCallBack(uint8_t id) {
		SwapDelete(m_TransformCallbacks, id);
	}

	void Entity::DispatchTransformCallBack() {
		UpdateWorldTransform();

		for (auto& func : m_TransformCallbacks) {
			func(m_Position, m_Rotation, m_Scale, m_WorldTransform);
		}
	}

	void Entity::SetParent(Entity* parent) {
		m_Parent = parent;
	}
}