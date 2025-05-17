#include <Engine/Core/LayerStack.h>
#include <Engine/Core/Log.h>

namespace Spike {

	LayerStack::LayerStack() {
		m_LayerInsert = m_Layers.begin();
	}

	LayerStack::~LayerStack() {

		CleanAll();
	}

	void LayerStack::CleanAll() {

		for (Layer* layer : m_Layers)
			delete layer;
	}

	void LayerStack::PushLayer(Layer* layer) {

		layer->OnAttach();
		m_LayerInsert = m_Layers.emplace(m_LayerInsert, layer);
	}

	void LayerStack::PopLayer(Layer* layer) {
		layer->OnDetach();

		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end()) {
			m_Layers.erase(it);
			m_LayerInsert--;
		}
	}


	void LayerStack::PushOverlay(Layer* overlay) {

		overlay->OnAttach();
		m_Layers.emplace_back(overlay);

		m_LayerInsert = m_Layers.begin();
	}

	void LayerStack::PopOverlay(Layer* overlay) {

		overlay->OnDetach();

		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end()) {
			m_Layers.erase(it);
		}
	}
}