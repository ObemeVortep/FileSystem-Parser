#include "main.h"

int main() {
    // initialize number - count of threads to work
    FSParser fsParser(COUNT_OF_THREADS_TO_WORK);

    auto startTime = std::chrono::high_resolution_clock::now();
    if (fsParser.CacheFS() != 0) {
        return -1;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Cache time: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime) << std::endl;

    // 1st argument - formats, 2nd argument - depth of search (0 if infinity), 3th - output directory (catalog will be created auto)
    fsParser.ConfigureSearch({ L".pdf" }, 0, L"C:\\Founded Files");

    startTime = std::chrono::high_resolution_clock::now();
    if (fsParser.SaveAllFilesByFormat() != 0) {
        return -2;
    }
    endTime = std::chrono::high_resolution_clock::now();

    std::cout << "FS Formats save time: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime) << std::endl;

    int x;
    std::cin >> x;

    return 0;
}