// #pragma once
//
// #include "string.hpp"
// #include "system_types.h"
//
// #include <filesystem>
// #include <format>
// #include <vector>
//
// namespace toaster::filesystem
// {
// 	using Path = std::filesystem::path;
//
// 	TST_LIB_API auto getWorkingDirectory() -> Path;
// 	TST_LIB_API auto setWorkingDirectory(const Path &p_dir) -> void;
//
// 	TST_LIB_API auto createDirectory(const Path &p_dir) -> void;
//
// 	TST_LIB_API auto exists(const Path &p_path) -> bool;
//
// 	TST_LIB_API auto readBinary(const Path &p_path) -> std::vector<uint32>;
// 	TST_LIB_API auto readFile(const Path &p_path) -> String;
//
// 	TST_LIB_API auto readFileAndSkipBOM(const Path &p_path) -> String;
// 	TST_LIB_API auto writeFile(const Path &p_path, const String &p_data) -> void;
//
// 	// I will add more
// 	enum class EFileType
// 	{
// 		eText,
// 		eMesh,
// 		eImage,
// 		eEnvironmentMap,
// 		eVideo,
// 		eOther
// 	};
//
// 	TST_LIB_API auto getFileType(const Path &p_path) -> EFileType;
// 	TST_LIB_API auto getFileTypeString(EFileType p_file_type) -> String;
// }
//
//
// template<>
// struct std::formatter<toaster::filesystem::Path>
// {
// 	constexpr auto parse(format_parse_context &p_ctx)
// 	{
// 		return p_ctx.begin();
// 	}
//
// 	auto format(const toaster::filesystem::Path &p_path, format_context &p_ctx) const
// 	{
// 		return format_to(p_ctx.out(), "{}", p_path.string());
// 	}
// };
