#include "main.h"

int main() {
    // initialize number - count of threads to work
    FSParser fsParser(8);

    // 1st argument - formats, 2nd argument - depth of search (0 if infinity), 3th - out directory (create auto)
    fsParser.ConfigureSearch({ L".pdf" }, 0, L"C:\\Founded Files");

    auto startTime = std::chrono::high_resolution_clock::now();
    if (fsParser.StartSearch() != 0) {
        std::cerr << "Error while searching" << std::endl;
        return -1;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "Execution time: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
        << std::endl;

    int x;
    std::cin >> x;

    return 0;
}