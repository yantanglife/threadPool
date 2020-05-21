#pragma once

#ifdef __linux__
#include <semaphore.h>
#define HAVE_SEM
#endif // __linux__


#include <mutex>
#include <condition_variable>

// On Linux, using sem_t from <semaphore>.
// Otherwise, using mutex and condition_variable to implement semaphore.
class Semaphore {
public:
    explicit Semaphore(unsigned int init = 0) {
#ifdef HAVE_SEM
        sem_init(&_sem, 0, init);
#else
        _count = init;
#endif // HAVE_SEM
    }

    ~Semaphore() {
#ifdef HAVE_SEM
        sem_destory(&_sem);
#endif // HAVE_SEM
    }

    void post(unsigned int n = 1) {
#ifdef HAVE_SEM
        while (n--) {
            sem_post(&_sem);
        }
#else
        std::unique_lock<std::mutex> lock(_mutex);
        _count += n;

        _condition.notify_all();
        
#endif // HAVE_SEM
    }

    void wait() {
#ifdef HAVE_SEM
        sem_wait(&_sem);
#else
        std::unique_lock<std::mutex> lock(_mutex);
        while (_count == 0) {
            _condition.wait(lock);
        }
        --_count;
#endif // HAVE_SEM
    }

private:
#ifdef HAVE_SEM
    sem_t _sem;
#else
    int _count;
    std::mutex _mutex;
    std::condition_variable_any _condition;
#endif
};