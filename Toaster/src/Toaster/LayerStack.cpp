#include "tstpch.h"
#include "LayerStack.hpp"


namespace tst
{
	void LayerStack::pushLayer(RefPtr<Layer> layer)
	{
		// Insert the layer at the current insert position
		m_layers.emplace(m_layers.begin() + m_layerInsertPos, layer);
		m_layerInsertPos++;
	}

	void LayerStack::pushOverlay(RefPtr<Layer> overlay)
	{
		m_layers.emplace_back(overlay);
	}

	void LayerStack::popLayer(RefPtr<Layer> layer)
	{
		// Find the layer in the stack and remove it
		auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		if (it != m_layers.end())
		{
			m_layers.erase(it);
			m_layerInsertPos--;
		}
	}

	void LayerStack::popOverlay(RefPtr<Layer> overlay)
	{
		auto it = std::find(m_layers.begin(), m_layers.end(), overlay);
		if (it != m_layers.end())
		{
			m_layers.erase(it);
		}
	}


}
