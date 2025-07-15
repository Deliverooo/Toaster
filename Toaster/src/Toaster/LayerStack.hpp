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

		void pushLayer(std::shared_ptr<Layer> layer);
		void pushOverlay(std::shared_ptr<Layer> overlay);
		void popLayer(std::shared_ptr<Layer> layer);
		void popOverlay(std::shared_ptr<Layer> overlay);

		std::vector<std::shared_ptr<Layer>>::iterator begin() { return m_layers.begin(); }
		std::vector<std::shared_ptr<Layer>>::iterator end() { return m_layers.end(); }

	private:

		std::vector<std::shared_ptr<Layer>> m_layers;
		// This iterator points to the position where the next layer will be inserted
		uint32_t m_layerInsertPos = 0;

	};

	
}
