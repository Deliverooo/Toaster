#pragma once

#include "tstpch.h"
#include "Layer.hpp"


namespace tst
{
	class TST_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void pushLayer(Layer* layer);
		void pushOverlay(Layer* overlay);
		void popLayer(Layer* layer);
		void popOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return m_layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_layers.end(); }

	private:

		std::vector<Layer*> m_layers;
		// This iterator points to the position where the next layer will be inserted
		uint32_t m_layerInsertPos = 0;

	};

	
}
