#include "tstpch.h"
#include "OpenGlTestLayer.h"
#include "glad/glad.h"
#include "Toaster/Core/Application.hpp"
#include "stb_image.h"


namespace tst
{
	OpenGlTestLayer::OpenGlTestLayer()
	{
		
	}

	OpenGlTestLayer::~OpenGlTestLayer()
	{
		
	}

	TstImageData loadImage(const char* filepath)
	{
		TstImageData result;
		result.data = stbi_load(filepath, &result.width, &result.height, &result.colourChannels, 3);

		if (!result.data)
		{
			TST_CORE_ERROR("Failed To Load Image {0}", filepath);
		} else
		{
			TST_CORE_INFO("Successfully Loaded Image {0}", filepath);
		}

		return result;
	}

	void OpenGlTestLayer::onAttach()
	{

		auto image = loadImage("C:/Users/oocon/Downloads/Orbo_blue.png");
	}

	void OpenGlTestLayer::onUpdate(DeltaTime dt)
	{
	}

	void OpenGlTestLayer::onEvent(Event& e)
	{
	}

    void OpenGlTestLayer::onDetach()
    {

    }
}
