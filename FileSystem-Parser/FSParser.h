#ifndef _FS_PARSER_H
#define _FS_PARSER_H

/*
*	Finding files using Win API and STL
*/

#include <vector>
#include <string>
#include <unordered_set>
#include <future>
#include <semaphore>
#include <deque>
#include <mutex>
#include <fstream>

/*
*   a struct, that include needed file information
*/
class FileStructure {
public:
	FileStructure(const std::wstring& filePath, const std::wstring& fileName) : path(filePath), name(fileName) {}
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

	bool operator==(const FileStructure& other) const {
		return name == other.name && path == other.path;
	}

	bool operator<(const FileStructure& other) const {
		if (name != other.name) {
			return name < other.name;
		}
		return path < other.path;
	}

	std::wstring name;
	std::wstring path;
	std::vector<char> binaryData;
};

/*
*	for unordered_set we need to create our hash function for FileStructure
*/
namespace std {
	template<>
	struct hash<FileStructure> {
		size_t operator()(const FileStructure& fs) const {
			size_t h1 = std::hash<std::wstring>()(fs.name);
			size_t h2 = std::hash<std::wstring>()(fs.path);
			return h1 ^ (h2 << 1); // Комбинируем хэши
		}
	};
}

/*
*	a class that searches for files with the specified formats and saves them (if necessary) on the computer
*/
class FSParser {
public:
	FSParser(int maxTasks);

	// add format in fileFormats
	void ConfigureSearch(std::initializer_list<std::wstring> formats, int initDepth, std::wstring outPath);

	// search files, depth determines the number of folders inside which we will search for files
	int StartSearch();

private:
	// function, that enumerate and check files in current folder
	void ListDirectories(const std::wstring& path, const int currentDepth);
	void AsyncListDirectories(std::wstring path, const int currentDepth);

	// function, that check format and if need process file
	void ProcessFile(const std::wstring& path, const std::wstring& fileName);

	// prepare all before process
	bool PrepareBeforeProcess();

	// process all files
	void ProcessAllFiles();

	// save current file
	void SaveFile(const FileStructure& fileStructure);

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

	// Async fields we need
	std::counting_semaphore<1024> limitSemaphore; 
	std::vector<std::future<void>> asyncTasks;
	int maximumAsyncTasks;

	std::mutex freeTasksIdMutex;
	std::deque<int> freeTasksId;

};


#endif // _FS_PARSER_H