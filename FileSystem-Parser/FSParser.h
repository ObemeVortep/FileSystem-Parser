#ifndef _FS_PARSER_H
#define _FS_PARSER_H

/*
*	Finding files using Win API and STL
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <future>
#include <semaphore>
#include <deque>
#include <mutex>
#include <fstream>

#include "FSParser_defines.h"
#include "FSParser_typedefs.h"

/*
*	a class that searches for files with the specified formats and saves them (if necessary) on the computer
*/
class FSParser {
public:
	FSParser(int maxTasks);

	// cache all files
	int CacheFS(DWORD flags);

	// search files, depth determines the number of folders inside which we will search for files
	int SaveAllFilesByFormat(std::initializer_list<std::wstring> formats, int initDepth, std::wstring tOutPath, DWORD flags);

	// function that find file by name and return vector of pathes
	std::vector<std::wstring> FindFileByName(std::wstring fileName);

	// function, that check if file format correct by fileFormats or with format
	bool CheckFileFormat(const std::wstring& fileName);

	// function that return file format (if exists)
	std::wstring GetFileFormat(const std::wstring& fileName);

	// function, that check format and if valid - save it in savedFiles
	void SaveFileInStructure(const std::wstring& path, const std::wstring& fileName);
	
	// function that cache fileName and path in cachedFiles
	void CacheFileInStructure(const std::wstring& path, const std::wstring& fileName);

private:

	// const undefined format string
	const std::wstring UNDEFINED_FORMAT;


	// Configure search
	void ConfigureSearch(std::initializer_list<std::wstring> formats, int initDepth, std::wstring outPath);

	// Clear after search
	void ClearSearchConfiguration();

	// async variant of SaveFileInStructure
	void AsyncSaveFileInStructure(std::wstring path, std::wstring fileName);
	
	// unordered map, first -> format, second -> FilePathInfo
	CachedFiles cachedFiles;
	std::mutex cachedFilesMutex;
	
	// function, that enumerate and check files in current folder
	template<typename Callable>
	void ListDirectories(const std::wstring& path, const int currentDepth, Callable processFunction);

	// async ListDirectories version
	template<typename Callable>
	void AsyncListDirectories(std::wstring path, const int currentDepth, Callable processFunction);

	// prepare all before process
	bool PrepareBeforeSaveOnFS();

	// save cached formats
	void SaveCachedFilesInStructure();

	// sort cached files
	void SortCachedStructure();

	// process all files
	void SaveAllFilesOnFS();

	// save current file
	void SaveFileOnFS(const FileStructure& fileStructure);

	// function, that find logical drives
	bool FindDrives();

	// function that wait until all threads end
	void WaitForThreads();
	
	// PC logical drives
	std::vector<wchar_t> drives;

	// File formats, we are looking for
	std::unordered_set<std::wstring> fileFormats;
	
	// Output file path
	std::wstring outPath;

	// Depth of searching (when 0 - infinity)
	int depth;

	// Files, that were found
	std::vector<FileStructure> savedFiles;
	std::mutex savedFilesMutex;

	int GetFreeThreadId();
	void FreeThread(int threadId);

	// Async fields we need
	std::counting_semaphore<1024> limitSemaphore; 
	std::vector<std::future<void>> asyncTasks;
	int maximumAsyncTasks;

	std::mutex freeTasksIdMutex;
	std::deque<int> freeTasksId;

	/*
	*	check functions
	*/
	inline bool IsDrivesInited() { return !drives.empty(); }
	inline bool IsFSCached() { return !cachedFiles.empty(); }
	inline bool IsFormatsInited() { return !fileFormats.empty(); }
	inline bool IsDepthInited() { return depth >= 0; }
	inline bool IsAsyncTasksInited() { return maximumAsyncTasks > 0; }

	/*
	*	debug fields/functions
	*/
	void StartTimeCount();
	void StopTimeCount(std::wstring outString);

	std::chrono::steady_clock::time_point startPoint;
	std::chrono::steady_clock::time_point endPoint;
};

template<typename Callable>
void FSParser::AsyncListDirectories(std::wstring newPath, const int currentDepth, Callable processFunction) {
	// if free thread exists - async call
	limitSemaphore.acquire();

	// get free thread ID
	int newThreadId = GetFreeThreadId();

	asyncTasks[newThreadId] = std::async(std::launch::async, [this, pathNeed = std::move(newPath), currentDepth, newThreadId, processFunction]() {
		ListDirectories(pathNeed, currentDepth, processFunction);

		FreeThread(newThreadId);

		limitSemaphore.release();
		});
}

template<typename Callable>
void FSParser::ListDirectories(const std::wstring& path, const int currentDepth, Callable processFunction) {
	// if currentDepth > depth (and depth != 0) end searching
	if (depth != 0 && currentDepth > depth) return;

	WIN32_FIND_DATA findFileData;
	HANDLE hFind;

	// forming a mask search
	std::wstring searchPath = path + L"\\*";

	// find first file/fodler
	hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {

		// no more files in folder or access denied (if no admin privilegies)
		return;
	}

	do {
		// skip "." (current dir) and ".."(parrent dir)
		if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0) {
			continue;
		}

		// check, if element is folder
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

			// if element is folder -> search deeper (don`t forget to increment currentDepth
			std::wstring newPath(searchPath.begin(), searchPath.end() - 1);
			newPath.append(findFileData.cFileName);

			// if we have free thread
			if (!freeTasksId.empty()) {

				AsyncListDirectories(std::move(newPath), currentDepth + 1, processFunction);
			}
			// if free thread no exists
			else {
				ListDirectories(newPath, currentDepth + 1, processFunction);
			}
		}
		else {
			// process founded file
			std::wstring fileName(findFileData.cFileName);
			processFunction(path, fileName);
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
}

#endif // _FS_PARSER_H