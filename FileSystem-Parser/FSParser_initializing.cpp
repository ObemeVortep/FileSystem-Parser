#include "FSParser.h"
#include <numeric>

FSParser::FSParser(int maxTasks) : depth(-1), maximumAsyncTasks(maxTasks), limitSemaphore(maxTasks), asyncTasks(maxTasks), freeTasksId(maxTasks) {
	std::iota(freeTasksId.begin(), freeTasksId.end(), 0);
	FindDrives();
};

bool FSParser::FindDrives() {
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

void FSParser::ConfigureSearch(std::initializer_list<std::wstring> formats, int initDepth, std::wstring tOutPath) {
	fileFormats = formats;
	depth = initDepth;
	outPath = std::move(tOutPath);
}

void FSParser::ClearSearchConfiguration() {
	ConfigureSearch({}, -1, std::move(std::wstring()));
}