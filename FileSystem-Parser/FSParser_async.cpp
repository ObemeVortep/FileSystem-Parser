#include "FSParser.h"

void FSParser::WaitForThreads() {
	for (auto& task : asyncTasks) {
		if (task.valid()) {
			task.get();
		}
	}
}


int FSParser::GetFreeThreadId() {
	std::lock_guard<std::mutex> lock(freeTasksIdMutex);
	int threadId = freeTasksId.front();
	freeTasksId.pop_front();

	return threadId;
}

void FSParser::FreeThread(int threadId) {
	std::lock_guard<std::mutex> lock(freeTasksIdMutex);
	freeTasksId.push_back(threadId);
}