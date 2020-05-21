#pragma once
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment (lib,"WS2_32")
#else
#include <time.h>
#endif // _WIN32


#define INSTANCE_IMP(class_name, ...) \
class_name &class_name::Instance() { \
    static std::shared_ptr<class_name> s_instance(new class_name(__VA_ARGS__)); \
    static class_name &s_insteanc_ref = *s_instance; \
    return s_insteanc_ref; \
}


class noncopyable {
protected:
	noncopyable() {}
	~noncopyable() {}
private:
	noncopyable(const noncopyable &that) = delete;
	noncopyable(noncopyable &&that) = delete;
	noncopyable &operator=(const noncopyable &that) = delete;
	noncopyable &operator=(noncopyable &&that) = delete;
};

std::string exePath();
std::string exeDir();
std::string exeName();

#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp);
void usleep(int micro_seconds);
void sleep(int second);
#endif