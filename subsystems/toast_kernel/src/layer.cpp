#include "toast_kernel/layer.hpp"
#include "toast_kernel/application.hpp"

namespace toaster
{
	auto IAppLayer::_register(Application *p_app) -> void
	{
		m_app       = p_app;
		m_renderCtx = p_app->m_renderCtx.get();
		m_inputCtx  = &p_app->getWindow().getInputCtx();
	}
}
