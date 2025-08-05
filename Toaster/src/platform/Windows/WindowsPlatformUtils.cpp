#include "tstpch.h"
#include "Toaster/Util/PlatformUtils.hpp"

#include <commdlg.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Toaster/Core/Application.hpp"

namespace tst
{
	std::string FileDialog::openFile(const char* filter)
	{
		OPENFILENAMEA openfilename;
		CHAR filesize[260] = { 0 };
		ZeroMemory(&openfilename, sizeof(OPENFILENAME));
		openfilename.lStructSize = sizeof(OPENFILENAME);
		openfilename.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::getInstance().getWindow().getWindow());
		openfilename.lpstrFile = filesize;
		openfilename.nMaxFile = sizeof(filesize);
		openfilename.lpstrFilter = filter;
		openfilename.nFilterIndex = 1;
		openfilename.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_NOCHANGEDIR;
		if(GetOpenFileNameA(&openfilename) == TRUE)
		{
			return openfilename.lpstrFile;
		}
		return {};
	}

	std::string FileDialog::saveFile(const char* filter)
	{
		OPENFILENAMEA openfilename;
		CHAR filesize[260] = { 0 };
		ZeroMemory(&openfilename, sizeof(OPENFILENAME));
		openfilename.lStructSize = sizeof(OPENFILENAME);
		openfilename.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::getInstance().getWindow().getWindow());
		openfilename.lpstrFile = filesize;
		openfilename.nMaxFile = sizeof(filesize);
		openfilename.lpstrFilter = filter;
		openfilename.nFilterIndex = 1;
		openfilename.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&openfilename) == TRUE)
		{
			return openfilename.lpstrFile;
		}
		return {};
	}
}
