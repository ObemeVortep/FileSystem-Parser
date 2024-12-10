#ifndef _FS_PARSER_TYPEDEFS_H
#define _FS_PARSER_TYPEDEFS_H

#include <cwctype>

/*
*   a struct, that include file information (name, path)
*/
class FilePathInfo {
public:
	FilePathInfo(const std::wstring& filePath, const std::wstring& fileName) : path(filePath), name(fileName) {}

	bool operator==(const FilePathInfo& other) const {
		return name == other.name && path == other.path;
	}

	bool operator<(const FilePathInfo& other) const {
		if (name != other.name) {
			return name < other.name;
		}
		return path < other.path;
	}

	std::wstring name;
	std::wstring path;
};

/*
*   a struct, that include FilePathInfo and binary data
*/
class FileStructure : public FilePathInfo {
public:
	FileStructure(const std::wstring& filePath, const std::wstring& fileName) : FilePathInfo(filePath, fileName) {}
	bool ReadDataFile() {
		// full path to the file
		std::wstring fullPath = path + L"\\" + name;

		// open file to read binary
		std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
		if (file) {

			// get fileSize
			std::streamsize fileSize = file.tellg();
			file.seekg(0, std::ios::beg);

			// resize vector and read data
			binaryData.resize(static_cast<size_t>(fileSize));
			file.read(binaryData.data(), fileSize);
			return true;
		}
		else {

			// error
			return false;
		}
	}

	std::vector<char> binaryData;
};

/*
*	a functor, that ignore upper and lower case in std::wstring
*/
struct CaseInsensitiveEqual {
	bool operator()(const std::wstring& lhs, const std::wstring& rhs) const {
		if (lhs.size() != rhs.size()) {
			return false;
		}

		for (size_t i = 0; i < lhs.size(); ++i) {
			if (std::towlower(lhs[i]) != std::towlower(rhs[i])) {
				return false;
			}
		}
		return true;
	}
};

/*
*	a functor, that ignore upper and lower case in std::wstring
*/
struct CaseInsensitiveHasher {
	size_t operator()(const std::wstring& str) const {
		size_t hash = 0;
		for (wchar_t ch : str) {
			hash = hash * 31 + std::towlower(ch);
		}
		return hash;
	}
};

typedef std::unordered_map<std::wstring, std::vector<FilePathInfo>, CaseInsensitiveHasher, CaseInsensitiveEqual> CachedFiles;


#endif // _FS_PARSER_TYPEDEFS_H