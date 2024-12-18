#include "FSParser.h"
#include <iostream>
#include <chrono>

void FSParser::StartTimeCount() {
	startPoint = std::chrono::high_resolution_clock::now();
}

void FSParser::StopTimeCount(std::wstring outString) {
	std::chrono::steady_clock::time_point endPoint = std::chrono::high_resolution_clock::now();
	std::wcout << outString << std::chrono::duration_cast<std::chrono::milliseconds>(endPoint - startPoint) << std::endl;
}