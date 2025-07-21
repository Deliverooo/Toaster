#pragma once

#include "tstpch.h"
#include "Layer.hpp"


namespace tst
{
	class TST_API LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack() {}

		void pushLayer(RefPtr<Layer> layer);
		void pushOverlay(RefPtr<Layer> overlay);
		void popLayer(RefPtr<Layer> layer);
		void popOverlay(RefPtr<Layer> overlay);

		std::vector<RefPtr<Layer>>::iterator begin() { return m_layers.begin(); }
		std::vector<RefPtr<Layer>>::iterator end() { return m_layers.end(); }

	private:

		std::vector<RefPtr<Layer>> m_layers;
		// This iterator points to the position where the next layer will be inserted
		uint32_t m_layerInsertPos = 0;

	};

	
}
