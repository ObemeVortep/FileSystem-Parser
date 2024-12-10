#include "main.h"

int main() {
    // initialize number - count of threads to work
    FSParser fsParser(COUNT_OF_THREADS_TO_WORK);
    
    // FSPARSER_TIME_EXECUTION means that we want check CacheFS time execution (unexpected, right?) 
    if (fsParser.CacheFS(FSPARSER_TIME_EXECUTION) != 0) {
        return -1;
    }

    // search all files by name, this requires FS caching
    std::vector<std::wstring> pathesOfFile = fsParser.FindFileByName(L"readme.me");
    for (const auto& path : pathesOfFile) {
        std::wcout << path << std::endl;
    }
 
    // 1st argument - formats, 2nd argument - depth of search (0 if infinity), 3th - output directory (catalog will be created auto)
    if (fsParser.SaveAllFilesByFormat({ L".pdf" }, 0, L"C:\\Founded Files", FSPARSER_TIME_EXECUTION) != 0) {
        return -2;
    }

    return 0;
}