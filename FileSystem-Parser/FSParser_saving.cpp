#include "FSParser.h"
#include <iostream>

#include "fs_help.hpp"

bool FSParser::CheckFileFormat(const std::wstring& fileName) {
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

void FSParser::SaveFileInStructure(const std::wstring& path, const std::wstring& fileName) {

	// create fileStructure and try to read data
	FileStructure fileStructure(path, fileName);
	if (fileStructure.ReadDataFile()) {

		// save file
		{
			std::lock_guard<std::mutex> lock(savedFilesMutex);
			savedFiles.push_back(std::move(fileStructure));
		}
	}
}

void FSParser::AsyncSaveFileInStructure(std::wstring path, std::wstring fileName) {
	limitSemaphore.acquire();

	int threadId = GetFreeThreadId();

	asyncTasks[threadId] = std::async(std::launch::async, [this, pathNeed = std::move(path), fileNameNeeded = std::move(fileName), threadId]() {
		SaveFileInStructure(pathNeed, fileNameNeeded);

		FreeThread(threadId);

		limitSemaphore.release();
		});
}

void FSParser::SaveCachedFilesInStructure() {

	// we go through each format
	for (const auto& format : fileFormats) {

		// if format exists
		auto it = cachedFiles.find(format);
		if (it != cachedFiles.end()) {
			
			// save all cached files
			for (const auto& fileInfo : it->second) {

				// if we can - use async
				if (!freeTasksId.empty()) {

					AsyncSaveFileInStructure(fileInfo.path, fileInfo.name);
				}
				// if not - :(
				else {

					SaveFileInStructure(fileInfo.path, fileInfo.name);
				}
			}
		}
	}
}

int FSParser::SaveAllFilesByFormat() {

	// check if fileFormats inited
	if (fileFormats.empty()) return -1;
	
	// check if drives inited
	if (drives.empty()) return -2;

	// check if depth inited
	if (depth < 0) return -3;

	// check if maximumAsyncTasks valid
	if (maximumAsyncTasks <= 0) return -4;

	// if FS cached - use cachedFiles
	if (!cachedFiles.empty()) {
		SaveCachedFilesInStructure();
	}
	// if not - start parse
	else {
		// when all checked - start search
		for (int d = 0; d < drives.size(); d++) {
			std::wstring path = std::wstring(1, drives[d]) + L":";
			AsyncListDirectories(std::move(path), 0, [this](const std::wstring& path, const std::wstring& fileName) {
				// save files if format correct
				if (this->CheckFileFormat(fileName)) {
					this->SaveFileInStructure(path, fileName);
				}
				});
		}
	}

	// wait until all tasks end
	WaitForThreads();

	// process all files
	SaveAllFilesOnFS();

	return 0;
}

void FSParser::SaveFileOnFS(const FileStructure& fileStructure) {
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

bool FSParser::PrepareBeforeSaveOnFS() {
	
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

void FSParser::SaveAllFilesOnFS() {
	std::cout << "Count of saved files: " << savedFiles.size() << std::endl;
	
	if (!PrepareBeforeSaveOnFS()) {
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
			// call async SaveFileOnFS
			SaveFileOnFS(file);
			
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
