#include <iostream>
#include <functional>
#include <random>

#include "threadPool.h"
#include "logger.h"
#include "util.h"


using namespace multi_thread;

int SEED = 2;

void add(const int &a, const int &b, int &c) {
    // c += a + b;
    /*
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned> u(1, 6);
    e.seed(time(NULL));
    */
    SEED *= SEED;
    srand(SEED);
    int t = rand() % 5;
    TraceL << "a = " << a << " b = "<< b << " " << t;
    sleep(t);
    c += a + b;
    TraceL << "c = " << c;
}

int main() {
    Logger::Instance().addChannel(std::make_shared<ConsoleChannel>());
    // Logger::Instance().addChannel(std::make_shared<FileChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

    ThreadPool pool(2);
    int ans = 0, b = 0, c = 0, d = 0;
    auto f = std::bind(add, 1, 2, std::ref(ans));
    pool.submit(f);
    auto g = std::bind(add, 1, 3, std::ref(b));
    pool.submit(g, true);
    auto h = std::bind(add, 1, 4, std::ref(c));
    pool.submit(h, true);
    auto j = std::bind(add, 1, 5, std::ref(d));
    pool.submit(j);
    // pool.clearTask();
    
    sleep(5);

    // DebugL << ans << b << c << d;
    return 0;
}

