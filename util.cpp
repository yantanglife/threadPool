#pragma once

#include "util.h"
#include "file.h"

#include <chrono>

#ifdef _WIN32
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif // _WIN32


std::string exePath() {
    char buffer[PATH_MAX * 2 + 1] = { 0 };
    int n = -1;
#ifdef _WIN32
    n = GetModuleFileNameA(NULL, buffer, sizeof(buffer));
#elif defined(__linux__)
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif // _WIN32

    std::string filePath;
    if (n <= 0) {
        filePath = "./";
    }
    else {
        filePath = buffer;
    }

#ifdef _WIN32
    // windows下把路径统一转换层unix风格，因为后续都是按照unix风格处理的
    for (auto &ch : filePath) {
        if (ch == '\\') {
            ch = '/';
        }
    }
#endif // _WIN32

    return filePath;
}

std::string exeDir() {
    auto path = exePath();
    return path.substr(0, path.rfind('/') + 1);
}

std::string exeName() {
    auto path = exePath();
    return path.substr(path.rfind('/') + 1);
}

#ifdef _WIN32
void sleep(int second) {
    Sleep(1000 * second);
}
void usleep(int micro_seconds) {
    struct timeval tm;
    tm.tv_sec = micro_seconds / 1000000;
    tm.tv_usec = micro_seconds % (1000000);
    select(0, NULL, NULL, NULL, &tm);
}
int gettimeofday(struct timeval *tp, void *tzp) {
    auto now_stamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    tp->tv_sec = now_stamp / 1000000LL;
    tp->tv_usec = now_stamp % 1000000LL;
    return 0;
}
#endif // _WIN32
