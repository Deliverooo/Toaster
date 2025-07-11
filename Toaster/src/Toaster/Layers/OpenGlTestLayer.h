#pragma once
#include "Toaster/Layer.hpp"

namespace tst
{
	class TST_API OpenGlTestLayer : public Layer
	{
	public:
        OpenGlTestLayer();
        ~OpenGlTestLayer();


        void onAttach() override;
        void onDetach() override {}
        void onUpdate() override;
        void onEvent(Event& e) override;

	private:
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 inPos;\n"
            "layout (location = 1) in vec3 inCol;\n"
            "out vec3 VertexColour;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(inPos.x, inPos.y, inPos.z, 1.0);\n"
            "   VertexColour = inCol;\n"
            "}\0";
        const char* fragmentShaderSource = "#version 330 core\n"
            "in vec3 VertexColour;\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(VertexColour, 1.0f);\n"
            "}\n\0";

        unsigned int m_shaderProgram;
        unsigned int m_Vao;
	};

}

