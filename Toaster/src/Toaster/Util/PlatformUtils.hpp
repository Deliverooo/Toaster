#pragma once

namespace tst
{
	class FileDialog
	{
	public:
		static std::string openFile(const char* filter = "All Files (*.*)\0*.*\0");

		static std::string saveFile(const char* filter = "All Files (*.*)\0*.*\0");
	};
}