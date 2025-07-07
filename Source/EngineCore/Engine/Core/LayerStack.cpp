#include <Engine/Core/LayerStack.h>
#include <Engine/Core/Log.h>

namespace Spike {

	LayerStack::LayerStack() {

		m_LayerInsertIndex = 0;
	}

	LayerStack::~LayerStack() {}

	void LayerStack::CleanAll() {

		for (Layer* layer : m_Layers)
			delete layer;

		m_Layers.clear();
	}

	void LayerStack::PushLayer(Layer* layer) {

		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		m_LayerInsertIndex++;

		layer->OnAttach();
	}

	void LayerStack::PopLayer(Layer* layer) {

		auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
		if (it != m_Layers.begin() + m_LayerInsertIndex)
		{
			layer->OnDetach();
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}


	void LayerStack::PushOverlay(Layer* overlay) {

		m_Layers.emplace_back(overlay);
		overlay->OnAttach();
	}

	void LayerStack::PopOverlay(Layer* overlay) {

		auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			overlay->OnDetach();
			m_Layers.erase(it);
		}
	}
}