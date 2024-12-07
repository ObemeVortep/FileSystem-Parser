#include "FSParser.h"

std::vector<std::wstring> FSParser::FindFileByName(std::wstring fileName) {
	std::vector<std::wstring> pathes;

	// if FS not cached - caching, than try to find our file
	if (!IsFSCached()) {
		CacheFS();
	}

	// get file format
	std::wstring fileFormat = GetFileFormat(fileName);
	if (!fileFormat.empty()) {

		// check if exists in cachedFiles
		auto files = cachedFiles.find(fileFormat);
		if (files != cachedFiles.end()) {

			auto beginIterator = files->second.begin();
			// go through vector
			while (true) {

				// find if exists
				auto it = std::find_if(beginIterator, files->second.end(), [&](const FilePathInfo& fileInfo) {
					return fileName == fileInfo.name;
					});

				// if no more - break
				if (it == files->second.end()) break;

				// push if path found and try to find more pathes
				pathes.push_back(it->path);

				// save iterator for next iteration
				beginIterator = ++it;
			}
		}
	}

	return pathes;
}

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

void FSParser::SortCachedStructure() {
	
	// iterate through all pairs
	for (auto& pair : cachedFiles) {

		// iterate through all vectors
		std::sort(pair.second.begin(), pair.second.end());
	}
}

int FSParser::CacheFS() {

	// check if drives inited
	if (!IsDrivesInited()) return -2;

	// check if maximumAsyncTasks valid
	if (!IsAsyncTasksInited()) return -4;

	// make depth temporal 0
	int savedDepth = depth;
	depth = 0;

	// when all checked - start caching
	for (int d = 0; d < drives.size(); d++) {
		std::wstring path = std::wstring(1, drives[d]) + L":";
		AsyncListDirectories(std::move(path), 0, 
			[this](const std::wstring& path, const std::wstring& fileName) {
			// cache file
			CacheFileInStructure(path, fileName);
			});
	}

	// wait until all files cached
	WaitForThreads();

	// sort cached structure
	SortCachedStructure();

	// return depth
	depth = savedDepth;

	return 0;
}