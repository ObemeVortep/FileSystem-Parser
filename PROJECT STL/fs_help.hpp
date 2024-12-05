#ifndef _FS_HELP_HPP
#define _FS_HELP_HPP

#include <string>
#include <filesystem>

__forceinline bool CreateDirectoryByFS(const std::wstring& path) {
	try {
		if (!std::filesystem::exists(path)) {
			if (!std::filesystem::create_directories(path)) {
				/* no created */
				return false;
			}
		}
	}
	catch (std::exception& e) {
		/* no created */
		return false;
	}
	
	return true;
}

#endif // _FS_HELP_HPP