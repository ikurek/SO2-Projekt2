#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

using namespace std;

mutex queue_mutex;
condition_variable queue_condition_variable;
queue<int> messageQueue;

bool finished = false;

void producer(int n) {
  int i = 0;
	while(true) {
			{
        lock_guard<mutex> queue_lock(queue_mutex);
  			messageQueue.push(i);
  			cout << "pushing " << i << endl;
        i++;
      }
      queue_condition_variable.notify_all();
      this_thread::sleep_for(chrono::milliseconds(1000));
	}
	queue_condition_variable.notify_all();
}

void consumer() {
	while (true) {
		unique_lock<mutex> queue_lock(queue_mutex);
		queue_condition_variable.wait(queue_lock, []{ return finished || !messageQueue.empty(); });
    std::cout << "consuming " << messageQueue.front() << std::endl;
    messageQueue.pop();
    queue_lock.unlock();
    this_thread::sleep_for(chrono::milliseconds(3000));
		if (finished) break;
	}
}

int main() {
	std::thread t1(producer, 10);
	std::thread t2(consumer);
	t1.join();
	t2.join();
	std::cout << "finished!" << std::endl;
}
