#include "FSParser.h"
#include <iostream>

#include "fs_help.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <numeric>

void FSParser::WaitForThreads() {
	for (auto& task : asyncTasks) {
		if (task.valid()) {
			task.get();
		}
	}
}

FSParser::FSParser(int maxTasks) : depth(-1), maximumAsyncTasks(maxTasks), limitSemaphore(maxTasks), asyncTasks(maxTasks), freeTasksId(maxTasks) {
	std::iota(freeTasksId.begin(), freeTasksId.end(), 0);
	FindDrives();
};


void FSParser::ConfigureSearch(std::initializer_list<std::wstring> formats, int initDepth, std::wstring tOutPath) {
	fileFormats = formats;
	depth = initDepth;
	outPath = std::move(tOutPath);
}

int FSParser::StartSearch() {

	// check if fileFormats inited
	if (fileFormats.empty()) return -1;
	
	// check if drives inited
	if (drives.empty()) return -2;

	// check if depth inited
	if (depth < 0) return -3;

	// check if maximumAsyncTasks valid
	if (maximumAsyncTasks <= 0) return -4;

	// when all checked - start search
	for (int d = 0; d < drives.size(); d++) {
		std::wstring path = std::wstring(1, drives[d]) + L":";
		AsyncListDirectories(std::move(path), 0);
	}

	// wait until all tasks end
	WaitForThreads();

	// process all files
	ProcessAllFiles();

	return 0;
}

void FSParser::SaveFile(const FileStructure& fileStructure) {
	std::wstring outputSummaryPath = outPath + L"\\" + fileStructure.path;

	// we must clear : (pos 2) from fileStructure.path
	size_t index = outPath.size() + 2;	// 1 == '\\', + 1 == drive, next is :
	outputSummaryPath.erase(index, 1);

	// now index point to first \, we increment and change all \ on _
	while (index < outputSummaryPath.size()) {
		index++;
		if (outputSummaryPath[index] == L'\\') outputSummaryPath[index] = L'_';
	}

	// create new directory
	CreateDirectoryByFS(outputSummaryPath);

	// create path with name
	std::wstring outputFilePath = outputSummaryPath + L"\\" + fileStructure.name;
	
	// save into the file
	std::ofstream file(outputFilePath, std::ios::binary);
	if (file.is_open()) {
		file.write(fileStructure.binaryData.data(), fileStructure.binaryData.size());
	}
}

bool FSParser::PrepareBeforeProcess() {
	
	// create output directory
	if (!CreateDirectoryByFS(outPath)) return false;

	// create drive output directory
	for (const auto& c : drives) {
		std::wstring drivePath = outPath + L"\\" + c;
		if (!CreateDirectoryByFS(drivePath)) return false;
	}

	/* created */
	return true;
}

void FSParser::ProcessAllFiles() {
	std::cout << "Count of saved files: " << savedFiles.size() << std::endl;
	
	if (!PrepareBeforeProcess()) {
		return;
	}

	// process files
	for (const auto& file : savedFiles) {

		// async saveFile
		limitSemaphore.acquire();

		int threadId;
		{
			std::lock_guard<std::mutex> lock(freeTasksIdMutex);
			threadId = freeTasksId.front();
			freeTasksId.pop_front();
		}

		asyncTasks[threadId] = std::async(std::launch::async, [this, file, threadId]() {
			// call async SaveFile
			SaveFile(file);
			
			{
				std::lock_guard<std::mutex> lock(freeTasksIdMutex);
				freeTasksId.push_back(threadId);
			}
			limitSemaphore.release();
			});
	}

	WaitForThreads();
	
	// Clear unordered set
	std::vector<FileStructure>().swap(savedFiles);
}

void FSParser::ProcessFile(const std::wstring& path, const std::wstring& fileName) {
	auto dotPos = fileName.find_last_of(L'.');
	if (dotPos != std::wstring::npos) {

		// check file format
		if (fileFormats.count(fileName.substr(dotPos))) {

			// create fileStructure and try to read data
			FileStructure fileStructure(path, fileName);
			if (fileStructure.ReadDataFile()) {

				// save file
				savedFiles.push_back(std::move(fileStructure));
			}
		}
	}
}

void FSParser::AsyncListDirectories(std::wstring newPath, const int currentDepth) {
	// if free thread exists - async call
	limitSemaphore.acquire();

	// get free thread ID
	int newThreadId;
	{
		std::lock_guard<std::mutex> lock(freeTasksIdMutex);
		newThreadId = freeTasksId.front();
		freeTasksId.pop_front();

	}
	asyncTasks[newThreadId] = std::async(std::launch::async, [this, pathNeed = std::move(newPath), currentDepth, newThreadId]() {
		ListDirectories(pathNeed, currentDepth);

		{
			std::lock_guard<std::mutex> lock(freeTasksIdMutex);
			freeTasksId.push_back(newThreadId);
		}

		limitSemaphore.release();
		});
}

void FSParser::ListDirectories(const std::wstring& path, const int currentDepth) {
	// if currentDepth > depth (and depth != 0) end searching
	if (depth != 0 && currentDepth > depth) return;

	WIN32_FIND_DATA findFileData;
	HANDLE hFind;

	// forming a mask search
	std::wstring searchPath = path + L"\\*";

	// find first file/fodler
	hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		
		// no more files in folder or not enough access (if no admin privilegies)
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
				AsyncListDirectories(std::move(newPath), currentDepth + 1);
			}
			// if free thread no exists
			else {
				ListDirectories(newPath, currentDepth + 1);
			}
		}
		else {
			// process founded file
			std::wstring fileName(findFileData.cFileName);
			ProcessFile(path, fileName);
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
}

bool FSParser::FindDrives(){
	/*
	*	GetLogicalDrives return bitmask of drives
	*	every bit match one logic drive (from a to z)
	*/
	DWORD drivesBitmask = GetLogicalDrives();
	if (drivesBitmask == 0) {
		return false;
	}

	int driveCount = 0;
	for (char drive = 'A'; drive <= 'Z'; ++drive) {

		// Check the least significant bit
		if (drivesBitmask & 1) {

			// Don't worry, english letters in UTF and ASCII are the same
			drives.push_back(drive);
		}

		// Shift one bit
		drivesBitmask >>= 1;
	}

	return true;
}