# FSParser – Multithreaded File System Parser (C++ with STL)

This project was developed in December 2024, shortly after I completed my first large-scale low-level C++ project(10k+ LOC, developed solo from July to November).  

### 📚 Motivation

During my first project, I worked almost entirely without STL (and in some modules, even without the C runtime library). While I gained deep experience in low-level C++ — including memory management, multithreading, process injection, socket-level networking, and cryptography — I realized I lacked exposure to **high-level, idiomatic C++ with STL**.

To fill this gap, I studied Scott Meyers’ *“Effective STL”* thoroughly and simultaneously started this project as a practical application of what I was learning.

### 🛠 Project Summary

FSParser is a multithreaded file system parser built to test and apply various STL features and high-level C++ design.

#### 🔍 Features:
- File search by name
- File search by extension
- Configurable multithreaded execution
- File system caching for faster re-parsing
- Built-in search performance profiling

#### 🧩 Code Structure:
- `FSParser_async.cpp` – multithreading and task dispatch
- `FSParser_caching.cpp` – caching mechanism
- `FSParser_debug.cpp` – logging and profiling tools
- `FSParser_initializing.cpp` – project setup and path scanning
- `FSParser_saving.cpp` – save/export functionality

> 🔒 All core logic was written from scratch — no code reuse from other repositories or tutorials.

### ⚡ Performance

On my machine (SSD 256GB + HDD 2TB), the parser caches both disks fully in 15–20 seconds. After caching, file search is nearly instant (a few milliseconds per query).

---

Thanks for reading!
