#include <thread>

#include "semaphore.h"
#include "logger.h"
#include "util.h"


// Using "semaphore.h" to test Production consumer model.


#define NUM 5

int queue[NUM];
Semaphore blank_number(NUM), product_number;

void Producer() {
	int i = 0;
	while (1) {
		blank_number.wait();
		queue[i] = rand() % 1000 + 1;
		InfoL << "Product---" << queue[i];
		product_number.post();
		i = (i + 1) % NUM;
		sleep(rand() % 2);
	}
}

void Consumer() {
	int i = 0;
	while (1) {
		product_number.wait();
		TraceL << "Consumer---" << queue[i];
		queue[i] = 0;
		blank_number.post();
		i = (i + 1) % NUM;
		sleep(rand() % 3);
	}
}

/*
int main() {
	auto channel = std::make_shared<ConsoleChannel>();
	channel->setDetail(false);
	Logger::Instance().addChannel(channel);
	Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());
	std::thread p(Producer);
	std::thread c(Consumer);
	p.join();
	c.join();
	
}
*/