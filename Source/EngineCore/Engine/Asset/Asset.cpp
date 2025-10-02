#include <Engine/Asset/Asset.h>

Spike::AssetRegistry* Spike::GRegistry = nullptr;
namespace Spike {

	void Asset::AddRef() const {
		m_Counter++;
	}

	void Asset::Release() const {
		m_Counter--;

		if (m_Counter == 1) {
			GRegistry->UnloadAsset(m_ID);
		}
		if (m_Counter == 0) {
			delete this;
		}
	}
}