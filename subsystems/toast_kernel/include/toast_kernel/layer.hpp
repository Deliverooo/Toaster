#pragma once

#include "toast_kernel.hpp"
#include "toast_gpu/fwd.hpp"

namespace toaster::knl
{
	class Event;
	struct Application;

	struct TST_KERNEL_API AppLayer
	{
		using InitFn     = void(*)(Application *, void *);
		using DestroyFn  = void(*)(Application *, void *);
		using OnRenderFn = void(*)(Application *, void *, gpu::CommandListHandle);
		using OnUpdateFn = void(*)(Application *, void *, float32);
		using OnEventFn  = void(*)(Application *, void *, Event &);

		InitFn     initFn{nullptr};
		DestroyFn  destroyFn{nullptr};
		OnRenderFn onRenderFn{nullptr};
		OnUpdateFn onUpdateFn{nullptr};
		OnEventFn  onEventFn{nullptr};

		void *userData{nullptr}; // This could be a stack of heap pointer, as it is managed by the end-user
	};
}
