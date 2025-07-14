#pragma once
#include "Toaster/Layer.hpp"

namespace tst
{
    struct TstImageData
    {
        std::optional<unsigned char*> data;
        int width;
        int height;
        int colourChannels;
    };

    struct TstImage 
    {
        TstImageData data;
        const char* filepath;
    };

	class TST_API OpenGlTestLayer : public Layer
	{
	public:
        OpenGlTestLayer();
        ~OpenGlTestLayer();


        void onAttach() override;
        void onDetach() override;
        void onUpdate() override;
        void onEvent(Event& e) override;

	private:
        const char* vertexShaderSource = "#version 450 core\n"
            "layout(location = 0) in vec3 aPos;\n"
            "layout(location = 1) in vec3 aColor;\n"
            "layout(location = 2) in vec2 aTexCoord;\n"
            "out vec3 ourColor;\n"
            "out vec2 TexCoord;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = vec4(aPos, 1.0);\n"
            "    ourColor = aColor;\n"
            "    TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
            "}\n";

        const char* fragmentShaderSource = "#version 450 core\n"
            "out vec4 FragColor;\n"
	        "in vec3 ourColor;\n"
	        "in vec2 TexCoord;\n"
	        "uniform sampler2D texture1;\n"
	        "uniform sampler2D texture2;\n"
	        "void main()\n"
	        "{\n"
	        "    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);\n"
	        "}\n";

        unsigned int m_shaderProgram;
        unsigned int m_Vao;

        TstImage m_image1;
        TstImage m_image2;

	};

}

