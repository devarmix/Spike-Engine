#include <Engine/World/Components.h>
#include <Engine/Core/Application.h>
#include <Engine/Renderer/FrameRenderer.h>

#define INVALID_BATCH_IDX UINT32_MAX

namespace Spike {

	void HierarchyComponent::SetParent(Entity parent) {
		assert(m_Self != parent && "Trying to parent entity to self, which is forbidden!");

		if (m_Parent) {
			auto& parentComp = m_Parent.GetComponent<HierarchyComponent>();

			for (int i = 0; i < parentComp.m_Children.size(); i++) {
				auto& child = parentComp.m_Children[i];

				if (child == m_Self) {
					SwapDelete(parentComp.m_Children, i);
					break;
				}
			}

			if (!parent) {
				m_Owner->SetEntityRoot(m_Self);
			}
		}

		m_Parent = parent;
		if (parent) {
			auto& parentComp = parent.GetComponent<HierarchyComponent>();
			parentComp.m_Children.push_back(m_Self);

			auto& parentTransform = parent.GetComponent<TransformComponent>();
			m_Self.GetComponent<TransformComponent>().UpdateParentTransform(parentTransform.GetWorldTranform());
		}
		else {
			m_Self.GetComponent<TransformComponent>().UpdateParentTransform(Mat4x4(1.f));
		}
	}

	HierarchyComponent::~HierarchyComponent() {
		for (auto& c : m_Children) {
			c.Destroy();
		}
	}

	TransformComponent::TransformComponent(Entity self) : 
		BaseEntityComponent(self),
		m_Position(0.f, 0.f, 0.f),
	    m_Rotation(0.f, 0.f, 0.f),
	    m_Scale(1.f, 1.f, 1.f) 
	{
		UpdateTransformMatrix();
	}

	void TransformComponent::SetPosition(const Vec3& value) {
		m_Position = value;
		UpdateTransformMatrix();
	}

	void TransformComponent::SetRotation(const Vec3& value) {
		m_Rotation = value;
		UpdateTransformMatrix();
	}

	void TransformComponent::SetScale(const Vec3& value) {
		m_Scale = value;
		UpdateTransformMatrix();
	}

	void TransformComponent::UpdateTransformMatrix(const Mat4x4* parentMat) {

		Mat4x4 T = glm::translate(Mat4x4(1.0f), m_Position);

		Quaternion pitchRotation = glm::angleAxis(m_Rotation.x, Vec3{ 1.f, 0.f, 0.f });
		Quaternion yawRotation = glm::angleAxis(m_Rotation.y, Vec3{ 0.f, 1.f, 0.f });
		Quaternion rollRotation = glm::angleAxis(m_Rotation.z, Vec3{ 0.f, 0.f, 1.f });

		Mat4x4 R = glm::toMat4(yawRotation * pitchRotation * rollRotation);
		Mat4x4 S = glm::scale(glm::mat4(1.0f), m_Scale);

		m_WorldTransform = T * R * S;
		if (parentMat) {
			m_WorldTransform *= *parentMat;
		}

		// call callbacks on change
		{
			if (m_Self.HasComponent<StaticMeshComponent>()) {
				auto& comp = m_Self.GetComponent<StaticMeshComponent>();
				comp.OnEntityMove(m_Position, m_Rotation, m_Scale, m_WorldTransform);
			}

			if (m_Self.HasComponent<LightComponent>()) {
				auto& comp = m_Self.GetComponent<LightComponent>();
				comp.OnEntityMove(m_Position, m_Rotation, m_Scale, m_WorldTransform);
			}
		}
	}

	void TransformComponent::UpdateParentTransform(const Mat4x4& mat) {
		UpdateTransformMatrix(&mat);
	}


	StaticMeshProxy::StaticMeshProxy(RHIWorldProxy* wProxy, const Mat4x4& transform) : m_WorldProxy(wProxy), m_LastTransform(transform) {}

	StaticMeshProxy::~StaticMeshProxy() {
		for (int i = 0; i < m_DataIndices.size(); i++) {
			m_WorldProxy->ObjectsVB.Pop(m_DataIndices[i]);
			
			if (m_Materials.size() > 0) {
				if (m_Materials[i].second != INVALID_BATCH_IDX) {
					RemoveFromBatch(m_Materials[i].second);
				}
			}
		}
	}

	void StaticMeshProxy::OnTransformChange(const Mat4x4& newTransform) {
		m_LastTransform = newTransform;

		for (auto idx : m_DataIndices) {

			ObjectGPUData& obj = m_WorldProxy->ObjectsVB[idx];
			obj.GlobalTransform = m_LastTransform;
			obj.InverseTransform = glm::inverse(m_LastTransform);
		}
	}

	void StaticMeshProxy::PushMaterial(RHIMaterial* mat) {
		m_Materials.push_back({});
		SetMaterial(mat, m_Materials.size() - 1, true);
	}

	void StaticMeshProxy::RemoveFromBatch(uint32_t idx) {
		WorldDrawBatch& b = m_WorldProxy->Batches[idx];

		if (b.CommandsCount > 1) {
			b.CommandsCount--;
		}
		else {
			b.Shader = nullptr;
			b.CommandsCount = 0;
		}
	}

	uint32_t StaticMeshProxy::FindBatch(RHIShader* shader) {
		uint32_t idx = INVALID_BATCH_IDX;

		for (int i = 0; i < m_WorldProxy->Batches.size(); i++) {
			auto& b = m_WorldProxy->Batches[i];

			if (b.Shader == shader || !b.Shader) {
				idx = i;
				b.CommandsCount++;

				if (!b.Shader) b.Shader = shader;
			}
		}

		if (idx == INVALID_BATCH_IDX) {
			idx = (uint32_t)m_WorldProxy->Batches.size();
			m_WorldProxy->Batches.push_back(WorldDrawBatch{ .CommandsCount = 1, .Shader = shader });
		}

		return idx;
	}

	void StaticMeshProxy::PopMaterial() {
		RemoveFromBatch(m_Materials.back().second);

		if (m_DataIndices.size() >= m_Materials.size()) {
			m_WorldProxy->ObjectsVB[m_DataIndices[m_Materials.size() - 1]].MaterialBufferIndex = INVALID_SHADER_INDEX;
		}
		m_Materials.pop_back();
	}

	void StaticMeshProxy::SetMaterial(RHIMaterial* mat, uint32_t index, bool isNew) {
		auto& matHolder = m_Materials[index];
		matHolder.first = mat;

		// check if material actually influences any of submeshes
		if (m_DataIndices.size() > index) {
			bool needNewBatch = isNew;

			if (!needNewBatch) {
				if (matHolder.first->GetShader() != mat->GetShader()) {
					RemoveFromBatch(matHolder.second);
					needNewBatch = true;
				}
			}

			ObjectGPUData& obj = m_WorldProxy->ObjectsVB[m_DataIndices[index]];
			obj.MaterialBufferIndex = mat->GetDataIndex();

			if (needNewBatch) {
				matHolder.second = obj.DrawBatchID = FindBatch(mat->GetShader());
			}
		}
		else {
			matHolder.second = INVALID_BATCH_IDX;
		}
	}

	void StaticMeshProxy::SetMesh(RHIMesh* mesh) {
		// remove all not needed prev sub-mesh proxies
		for (int i = mesh->GetDesc().SubMeshes.size(); i < m_DataIndices.size(); i++) {
			m_WorldProxy->VisibilityQueue.Release(m_WorldProxy->ObjectsVB[m_DataIndices[i]].VisibilityIdx);
			m_WorldProxy->ObjectsVB.Pop(m_DataIndices[i]);

			if (m_Materials.size() > i) {
				RemoveFromBatch(m_Materials[i].second);
			}
		} 

		// add missing sub-mesh proxies
		for (int i = m_DataIndices.size(); i < mesh->GetDesc().SubMeshes.size(); i++) {
			m_DataIndices.push_back(m_WorldProxy->ObjectsVB.Push(ObjectGPUData{.VisibilityIdx = m_WorldProxy->VisibilityQueue.Grab()}));

			if (m_Materials.size() > i) {
				if (m_Materials[i].second == INVALID_BATCH_IDX) {
					m_Materials[i].second = FindBatch(m_Materials[i].first->GetShader());
				}
				else {
					m_WorldProxy->Batches[m_Materials[i].second].CommandsCount++;
				}
			}
		}
		 
		for (int i = 0; i < mesh->GetDesc().SubMeshes.size(); i++) {
			const SubMesh& sMesh = mesh->GetDesc().SubMeshes[i];

			ObjectGPUData& obj = m_WorldProxy->ObjectsVB[m_DataIndices[i]];
			obj.BoundsOrigin = Vec4(mesh->GetBoundsOrigin(), mesh->GetBoundsRadius());
			obj.GlobalTransform = m_LastTransform;
			obj.InverseTransform = glm::inverse(m_LastTransform);
			obj.FirstIndex = sMesh.FirstIndex;
			obj.IndexCount = sMesh.IndexCount;
			obj.IndexBufferAddress = mesh->GetIndexBuffer()->GetGPUAddress();
			obj.VertexBufferAddress = mesh->GetVertexBuffer()->GetGPUAddress();
			obj.MaterialBufferIndex = (m_Materials.size() > i) ? m_Materials[i].first->GetDataIndex() : INVALID_SHADER_INDEX;
			obj.DrawBatchID = (m_Materials.size() > i) ? m_Materials[i].second : INVALID_SHADER_INDEX;
		}
	}

	StaticMeshComponent::StaticMeshComponent(Entity entity) : BaseEntityComponent(entity), m_Mesh(nullptr) {
		m_Proxy = new StaticMeshProxy(entity.GetWorld()->GetProxy(), 
		    entity.GetComponent<TransformComponent>().GetWorldTranform());
	}

	StaticMeshComponent::~StaticMeshComponent() {
		GFrameRenderer->SubmitToFrameQueue([proxy = m_Proxy]() {
			delete proxy;
			});
	}

	void StaticMeshComponent::OnEntityMove(const Vec3& pos, const Vec3& rot, const Vec3& scale, const Mat4x4& world) {
		GFrameRenderer->SubmitToFrameQueue([=, proxy = m_Proxy]() {
			proxy->OnTransformChange(world);
			});
	}

	void StaticMeshComponent::SetMesh(Ref<Mesh> mesh) {
		RHIMesh* rhiMesh = mesh->GetResource();

		GFrameRenderer->SubmitToFrameQueue([rhiMesh, proxy = m_Proxy]() {
			proxy->SetMesh(rhiMesh);
			});
		m_Mesh = mesh;
	}

	void StaticMeshComponent::SetMaterial(Ref<Material> mat, uint32_t index) {
		RHIMaterial* rhiMat = mat->GetResource();

		GFrameRenderer->SubmitToFrameQueue([rhiMat, index, proxy = m_Proxy]() {
			proxy->SetMaterial(rhiMat, index);
			});
		m_Materials[index] = mat;
	}

	void StaticMeshComponent::PushMaterial(Ref<Material> mat) {
		RHIMaterial* rhiMat = mat->GetResource();
		
		GFrameRenderer->SubmitToFrameQueue([rhiMat, proxy = m_Proxy]() {
			proxy->PushMaterial(rhiMat);
			});
		m_Materials.push_back(mat);
	}

	void StaticMeshComponent::PopMaterial() {
		GFrameRenderer->SubmitToFrameQueue([proxy = m_Proxy]() {
			proxy->PopMaterial();
			});
		m_Materials.pop_back();
	}


	LightProxy::LightProxy(RHIWorldProxy* wProxy) 
		: m_WorldProxy(wProxy), m_DataIndex(~0u) {}

	LightProxy::~LightProxy() {
		m_WorldProxy->LightsVB.Pop(m_DataIndex);
	}

	void LightProxy::Init(const Vec3& pos, const Vec3& rot) {
		m_DataIndex = m_WorldProxy->LightsVB.Push(
			LightGPUData{.Position = Vec4(pos, 1.0f), .Direction = Vec4(rot, 1.0f)});
	}

	void LightProxy::SetIntensity(float value) {
		m_WorldProxy->LightsVB[m_DataIndex].Intensity = value;
	}

	void LightProxy::SetRange(float value) {
		m_WorldProxy->LightsVB[m_DataIndex].Range = value;
	}

	void LightProxy::SetColor(const Vec4& value) {
		m_WorldProxy->LightsVB[m_DataIndex].Color = value;
	}

	void LightProxy::SetDirection(const Vec3& dir) {
		m_WorldProxy->LightsVB[m_DataIndex].Direction = Vec4(dir, 1.0f);
	}

	void LightProxy::SetLightLinear(float value) {
		m_WorldProxy->LightsVB[m_DataIndex].LightLinear = value;
	}

	void LightProxy::SetLightConstant(float value) {
		m_WorldProxy->LightsVB[m_DataIndex].LightConstant = value;
	}

	void LightProxy::SetLightQuadratic(float value) {
		m_WorldProxy->LightsVB[m_DataIndex].LightQuadratic = value;
	}

	void LightProxy::SetType(uint32_t type) {
		m_WorldProxy->LightsVB[m_DataIndex].Type = type;
	}

	void LightProxy::SetInnerConeCos(float value) {
		m_WorldProxy->LightsVB[m_DataIndex].InnerConeCos = value;
	}

	void LightProxy::SetOuterConeCos(float value) {
		m_WorldProxy->LightsVB[m_DataIndex].OuterConeCos = value;
	}

	void LightProxy::SetPosition(const Vec3& pos) {
		m_WorldProxy->LightsVB[m_DataIndex].Position = Vec4(pos, 1.0f);
	}

	LightComponent::LightComponent(Entity entity) : BaseEntityComponent(entity) {
		m_Proxy = new LightProxy(entity.GetWorld()->GetProxy());

		Vec3 pos = m_Self.GetComponent<TransformComponent>().GetPosition();
		Vec3 rot = m_Self.GetComponent<TransformComponent>().GetRotation();

		GFrameRenderer->SubmitToFrameQueue([proxy = m_Proxy, pos, rot]() {
			proxy->Init(pos, rot);
			});
	}

	LightComponent::~LightComponent() {
		GFrameRenderer->SubmitToFrameQueue([proxy = m_Proxy]() {
			delete proxy;
			});
	}

	void LightComponent::OnEntityMove(const Vec3& pos, const Vec3& rot, const Vec3& scale, const Mat4x4& world) {
		GFrameRenderer->SubmitToFrameQueue([pos, rot, proxy = m_Proxy]() {
			proxy->SetPosition(pos);
			proxy->SetDirection(rot);
			});
	}

	void LightComponent::SetIntensity(float value) {
		GFrameRenderer->SubmitToFrameQueue([value, proxy = m_Proxy]() {
			proxy->SetIntensity(value);
			});
	}

	void LightComponent::SetRange(float value) {
		GFrameRenderer->SubmitToFrameQueue([value, proxy = m_Proxy]() {
			proxy->SetRange(value);
			});
	}

	void LightComponent::SetColor(const Vec4& value) {
		GFrameRenderer->SubmitToFrameQueue([value, proxy = m_Proxy]() {
			proxy->SetColor(value);
			});
	}

	void LightComponent::SetLightLinear(float value) {
		GFrameRenderer->SubmitToFrameQueue([value, proxy = m_Proxy]() {
			proxy->SetLightLinear(value);
			});
	}

	void LightComponent::SetLightConstant(float value) {
		GFrameRenderer->SubmitToFrameQueue([value, proxy = m_Proxy]() {
			proxy->SetLightConstant(value);
			});
	}

	void LightComponent::SetLightQuadratic(float value) {
		GFrameRenderer->SubmitToFrameQueue([value, proxy = m_Proxy]() {
			proxy->SetLightQuadratic(value);
			});
	}

	void LightComponent::SetType(ELightType type) {
		GFrameRenderer->SubmitToFrameQueue([type, proxy = m_Proxy]() {
			proxy->SetType((uint8_t)type);
			});
	}

	void LightComponent::SetInnerConeCos(float value) {
		GFrameRenderer->SubmitToFrameQueue([value, proxy = m_Proxy]() {
			proxy->SetInnerConeCos(value);
			});
	}

	void LightComponent::SetOuterConeCos(float value) {
		GFrameRenderer->SubmitToFrameQueue([value, proxy = m_Proxy]() {
			proxy->SetOuterConeCos(value);
			});
	}
}