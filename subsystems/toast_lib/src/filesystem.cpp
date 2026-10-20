// #include "toast_lib/filesystem.hpp"
// #include "toast_lib/toast_assert.h"
//
// #include <fstream>
//
// namespace toaster::filesystem
// {
// 	static auto skipByteOrderMark(std::istream &p_in) -> int
// 	{
// 		char test[4] = {0};
// 		p_in.seekg(0, std::ios::beg);
// 		p_in.read(test, 3);
// 		if (std::strcmp(test, "\xEF\xBB\xBF") == 0)
// 		{
// 			p_in.seekg(3, std::ios::beg);
// 			return 3;
// 		}
// 		p_in.seekg(0, std::ios::beg);
// 		return 0;
// 	}
//
// 	auto getWorkingDirectory() -> Path
// 	{
// 		return std::filesystem::current_path();
// 	}
//
// 	auto setWorkingDirectory(const Path &p_dir) -> void
// 	{
// 		std::filesystem::current_path(p_dir);
// 	}
//
// 	auto createDirectory(const Path &p_dir) -> void
// 	{
// 		std::filesystem::create_directory(p_dir);
// 	}
//
// 	auto exists(const Path &p_path) -> bool
// 	{
// 		return std::filesystem::exists(p_path);
// 	}
//
// 	auto readBinary(const Path &p_path) -> std::vector<uint32>
// 	{
// 		std::vector<uint32> result;
// 		std::ifstream       in{p_path, std::ios::in | std::ios::binary | std::ios::ate};
// 		if (in)
// 		{
// 			const auto file_size = in.tellg();
// 			result.resize(file_size / sizeof(uint32));
//
// 			in.seekg(0);
//
// 			in.read(reinterpret_cast<char *>(result.data()), file_size);
// 		}
// 		in.close();
// 		return result;
// 	}
//
// 	auto readFile(const Path &p_path) -> String
// 	{
// 		String        result;
// 		std::ifstream in{p_path, std::ios::in | std::ios::binary | std::ios::ate};
// 		if (in)
// 		{
// 			const auto file_size = in.tellg();
// 			result.resize(file_size);
//
// 			in.seekg(0);
//
// 			in.read(result.data(), file_size);
// 		}
// 		in.close();
// 		return result;
// 	}
//
// 	auto readFileAndSkipBOM(const Path &p_path) -> String
// 	{
// 		String        result;
// 		std::ifstream in{p_path, std::ios::in | std::ios::binary};
// 		if (in)
// 		{
// 			in.seekg(0, std::ios::end);
// 			auto      fileSize     = in.tellg();
// 			const int skippedChars = skipByteOrderMark(in);
//
// 			fileSize -= skippedChars - 1;
// 			result.resize(fileSize);
// 			in.read(result.data() + 1, fileSize);
//
// 			result[0] = '\t';
// 		}
// 		in.close();
// 		return result;
// 	}
//
// 	auto writeFile(const Path &p_path, const String &p_data) -> void
// 	{
// 		std::ofstream out{p_path, std::ios::out | std::ios::binary};
// 		if (out)
// 		{
// 			out.write(p_data.data(), static_cast<std::streamsize>(p_data.size()));
// 		}
// 		else
// 		{
// 			TST_PERMA_ASSERT_MSG(false, "What is dis");
// 		}
// 		out.close();
// 	}
//
// 	auto getFileType(const Path &p_path) -> EFileType
// 	{
// 		const String extension{p_path.extension().string()};
//
// 		if (extension == ".txt" || extension == ".md")
// 			return EFileType::eText;
//
// 		if (extension == ".fbx" || extension == ".obj" || extension == ".glb" || extension == ".gltf")
// 			return EFileType::eMesh;
//
// 		if (extension == ".png" || extension == ".jpg" || extension == ".bmp" || extension == ".jpeg")
// 			return EFileType::eImage;
//
// 		if (extension == ".exr" || extension == ".hdr")
// 			return EFileType::eEnvironmentMap;
//
// 		if (extension == ".mp4" || extension == ".mov" || extension == ".mkv")
// 			return EFileType::eVideo;
//
// 		return EFileType::eOther;
// 	}
//
// 	auto getFileTypeString(EFileType p_file_type) -> String
// 	{
// 		switch (p_file_type)
// 		{
// 			case EFileType::eText: return "Text";
// 			case EFileType::eMesh: return "Mesh";
// 			case EFileType::eImage: return "Image";
// 			case EFileType::eEnvironmentMap: return "Environment Map";
// 			case EFileType::eVideo: return "Video";
// 			case EFileType::eOther: return "Other";
// 		}
// 		return "Wat is dis?";
// 	}
// }
