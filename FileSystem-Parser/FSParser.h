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
#include <filesystem>

#include "FSParser_defines.h"
#include "FSParser_typedefs.h"

/*
*	a class that searches for files with the specified formats and saves them (if necessary) on the computer
*/
class FSParser {
	
	/*
	*	FSParser_initializing.cpp
	*	Functions and fields are responsible for initializing this class
	*/
public:
	// Constructor, initializes almost all fields
	FSParser(int maxTasks);
private:
	// Searches for drivers and writes letters into drives vector
	bool FindDrives();
	std::vector<wchar_t> drives;						// Contains drive letters
	// Assigns values ​​to the fields formats, depth, outPath. Required for CacheFS/SaveAllFilesByFormat
	void ConfigureSearch(std::initializer_list<std::wstring> formats, int initDepth, std::wstring outPath);
	// Calls ConfigureSearch with zero parameters to reset the configuration after the search
	void ClearSearchConfiguration();				
	std::unordered_set<std::wstring> fileFormats;		// The file format that we will search for and save in the SaveAllFilesByFormat function
	int depth;											// Determines how deep the ListDirectories function can go (when 0 - infinity)
	std::wstring outPath;								// Output file path for SaveAllFilesByFormat
	
	/*
	*	FSParser_async.cpp
	*	Functions and fields responsible for the correct operation of asynchronous threads
	*/
private:
	// The function blocks the current thread until each asynchronous thread has completed its work
	void WaitForThreads();
	// Gets the free thread ID from freeTasksId
	int GetFreeThreadId();
	// Returns the free thread ID in freeTasksId
	void FreeThread(int threadId);
	std::counting_semaphore<1024> limitSemaphore;		// Prevents thread races from occurring
	std::vector<std::future<void>> asyncTasks;			// "Pool" of threads, the number is initialized in the constructor using maxTasks
	int maximumAsyncTasks;								// Max count of threads, the number is initialized in the constructor using maxTasks
	std::mutex freeTasksIdMutex;						// Mutex for asynchronous access to freeTasksId
	std::deque<int> freeTasksId;						// Pool of free ID streams

	/*
	*	FSParser_caching.cpp
	*	Functions and fields responsible for file system caching
	*/
public:
	// The function caches the names and paths of all files on the system and stores them in cachedFiles for quick access
	int CacheFS(DWORD flags);
	// The function uses the cached system and searches for a file by name and returns an array of paths where the file was found. Caches the system if it has not been called.
	std::vector<std::wstring> FindFileByName(std::wstring fileName);
private:
	// A function that writes the path and file name to an internal field
	void CacheFileInStructure(const std::wstring& path, const std::wstring& fileName);
	// Sorts a vector of cached files (no practical use yet)
	void SortCachedStructure();
	CachedFiles cachedFiles;							// Cached files are stored here, the typedef is in FSParser_typedefs.h
	std::mutex cachedFilesMutex;						// Mutex for asynchronous access to cachedFiles

	/*
	*	FSParser_saving.cpp
	*	Functions and fields responsible for searching and writing files to the file system
	*/
public:
	// Searches for files in accordance with the passed parameters and saves them in outPath. If the system is cached - checks cachedFiles
	int SaveAllFilesByFormat(std::initializer_list<std::wstring> formats, int initDepth, std::wstring tOutPath, DWORD flags);
private:
	// If the filesystem is already cached, instead of reading it again, it looks at the cached files
	void SaveCachedFilesInStructure();
	// Saves a file (with binary content) to the internal savedFiles structure.
	void SaveFileInStructure(const std::wstring& path, const std::wstring& fileName);
	// Calls the SaveFileInStructure function asynchronously
	void AsyncSaveFileInStructure(std::wstring path, std::wstring fileName);
	// Creates convenient directories for outputting found files (using CreateDirectoryByFS)
	bool PrepareBeforeSaveOnFS();
	// Saves all files from savedFiles to the outPath specified by SaveAllFilesByFormat. Automatically clears savedFiles
	void SaveAllFilesOnFS();
	// A function responsible for writing a single file to outPath
	void SaveFileOnFS(const FileStructure& fileStructure);
	std::vector<FileStructure> savedFiles;				// Saved files when searching in SaveAllFilesByFormat
	std::mutex savedFilesMutex;							// Mutex for asynchronous access to savedFiles

	/*
	*	FSParser_debug.cpp
	*	Functions and fields responsible for debugging and performance indicators
	*/
private:
	// Starts timing and stores it in startPoint. Works when calling functions with the FSPARSER_TIME_EXECUTION flag
	void StartTimeCount();
	// Ends timing and displays the execution time of the function
	void StopTimeCount(std::wstring outString);
	std::chrono::steady_clock::time_point startPoint;	// Necessary for correct operation of StartTimeCount/StopTimeCount

	/*
	*	Functions responsible for direct work with the file system
	*/
private:
	// Creates directories (entirely) at the specified path
	inline bool CreateDirectoryByFS(const std::wstring& path) {
		try {
			if (!std::filesystem::exists(path)) {
				if (!std::filesystem::create_directories(path)) {
					// no created 
					return false;
				}
			}
		}
		catch (std::exception& e) {
			// no created 
			return false;
		}

		return true;
	}

	/*
	*	Functions and fields for working with file formats
	*/
private:
	// Checks if the file format is included in formats
	inline bool CheckFileFormat(const std::wstring& fileName) {
		auto dotPos = fileName.find_last_of(L'.');
		if (dotPos != std::wstring::npos) {

			// check file format
			if (fileFormats.count(fileName.substr(dotPos))) {

				// format correct
				return true;
			}
		}

		// incorrect format
		return false;
	}
	// The function returns the file format. If it is not present, the UNDEFINED_FORMAT field is returned
	inline std::wstring GetFileFormat(const std::wstring& fileName) {
		auto dotPos = fileName.find_last_of(L'.');
		if (dotPos != std::wstring::npos) {

			return fileName.substr(dotPos);
		}
		else {

			return std::wstring(UNDEFINED_FORMAT);
		}
	}
	const std::wstring UNDEFINED_FORMAT;				// Important field, initialized in the constructor in L"undefined", used in cases where the file does not have an extension

	/*
	*	Functions made for convenience to check the initialization of fields
	*/
private:
	inline bool IsDrivesInited() { return !drives.empty(); }
	inline bool IsFSCached() { return !cachedFiles.empty(); }
	inline bool IsFormatsInited() { return !fileFormats.empty(); }
	inline bool IsDepthInited() { return depth >= 0; }
	inline bool IsAsyncTasksInited() { return maximumAsyncTasks > 0; }

	/*
	*	Perhaps the most important functions that are actively used in CacheFS/SaveAllFilesByFormat for file system parsing
	*/
private:
	// The function parses the passed directory, calls the Callable function with each file found and continues searching in depth (if the depth allows)
	template<typename Callable>
	void ListDirectories(const std::wstring& path, const int currentDepth, Callable processFunction);
	// Calls the ListDirectories function asynchronously
	template<typename Callable>
	void AsyncListDirectories(std::wstring path, const int currentDepth, Callable processFunction);
};

/*
*	Template functions must be defined in the class header file
*/
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
/*
*	Template functions must be defined in the class header file
*/
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

#endif // _FS_PARSER_H