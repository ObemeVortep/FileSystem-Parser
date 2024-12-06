#include "FSParser.h"
#include <iostream>

void FSParser::CacheFileInStructure(const std::wstring& path, const std::wstring& fileName) {
	auto dotPos = fileName.find_last_of(L'.');
	std::wstring fileFormat;
	if (dotPos != std::wstring::npos) {
		fileFormat = std::move(fileName.substr(dotPos));
	}
	else {
		fileFormat = L"undefined";
	}
	FilePathInfo filePathInfo(path, fileName);
	{
		std::lock_guard<std::mutex> lock(cachedFilesMutex);
		cachedFiles[fileFormat].push_back(std::move(filePathInfo));
	}
}

int FSParser::CacheFS() {

	// check if drives inited
	if (drives.empty()) return -2;

	// check if maximumAsyncTasks valid
	if (maximumAsyncTasks < 0) return -4;

	// make depth temporal 0
	int savedDepth = depth;
	depth = 0;

	// when all checked - start caching
	for (int d = 0; d < drives.size(); d++) {
		std::wstring path = std::wstring(1, drives[d]) + L":";
		AsyncListDirectories(std::move(path), 0, 
			[this](const std::wstring& path, const std::wstring& fileName) {
			// cache file
			this->CacheFileInStructure(path, fileName);
			});
	}

	// wait until all files cached
	WaitForThreads();

	// return depth
	depth = savedDepth;

	return 0;
}