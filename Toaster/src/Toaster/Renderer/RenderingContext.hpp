#pragma once

namespace tst
{
	class TST_API RenderingContext
	{
	public:

		virtual ~RenderingContext(){}

		virtual void init() = 0;

		virtual void swapBuffers() = 0;

	protected:

	};
}
