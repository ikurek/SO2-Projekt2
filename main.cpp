#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <ncurses.h>
#include "Message.h"

using namespace std;

unsigned int maxQueueSize = 6;

mutex queue_mutex;
condition_variable queue_condition_variable;
queue<Message> messageQueue;
queue<Message> canceledQueue;
queue<Message> finishedQueue;
Message lastMessage = Message(-1);

bool finished = false;

int getRandomColourID() {
  return rand()% 4;
}

void producer(int n) {
  int i = 0;
	while(!finished) {
    lastMessage = Message(i);
    this_thread::sleep_for(chrono::milliseconds(1000));
			{
        if (messageQueue.size() >= maxQueueSize) {
          canceledQueue.push(lastMessage);
        } else {
          lock_guard<mutex> queue_lock(queue_mutex);
          messageQueue.push(lastMessage);
          queue_condition_variable.notify_all();
        }
        lastMessage = Message(-1);
        i++;
      }
	}
	queue_condition_variable.notify_all();
}

void consumer() {
	while (!finished) {
    this_thread::sleep_for(chrono::milliseconds(2000));
		unique_lock<mutex> queue_lock(queue_mutex);
		queue_condition_variable.wait(queue_lock, []{ return finished || !messageQueue.empty(); });
    finishedQueue.push(messageQueue.front());
    messageQueue.pop();
    queue_lock.unlock();
	}
}

void ui() {
  while(!finished) {
    clear();
    attron(COLOR_PAIR(0));
    mvprintw(1, 1, "               In:       ");
    mvprintw(2, 1, "-----              ------");
    mvprintw(3, 1, "|   |   =>         |    | Out:");
    mvprintw(4, 1, "-----              ------");
    mvprintw(5, 1, "Canceled: ");

    if (lastMessage.id != -1) {
      mvprintw(3, 7, lastMessage.symbol.c_str());
    } else {
      mvprintw(3, 8, " ");
    }

    queue<Message> copyOfMessageQueue = messageQueue;

    for(unsigned int i = 0; i < maxQueueSize; ++i) {
      if (!copyOfMessageQueue.empty()) {
        mvprintw(1, (19 + maxQueueSize - i), copyOfMessageQueue.front().symbol.c_str());
        copyOfMessageQueue.pop();
      } else {
        mvprintw(1, (19 + maxQueueSize - i), " ");
      }
    }
    
    queue<Message> copyOfFinishedQueue = finishedQueue;
    int counterFinished = 32;
    while(!copyOfFinishedQueue.empty()) {
      mvprintw(3, counterFinished, copyOfFinishedQueue.front().symbol.c_str());
      copyOfFinishedQueue.pop();
      counterFinished++;
    }

    queue<Message> copyOfCanceledQueue = canceledQueue;
    int counterCanceled = 11;
    while(!copyOfCanceledQueue.empty()) {
      mvprintw(5, counterCanceled, copyOfCanceledQueue.front().symbol.c_str());
      copyOfCanceledQueue.pop();
      counterCanceled++;
    }


    refresh();

    this_thread::sleep_for(chrono::milliseconds(100));
  }



}

void initNcursesPairs() {
  init_pair(0, COLOR_WHITE, COLOR_BLACK);
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);
  init_pair(4, COLOR_BLUE, COLOR_BLACK);

}

int main() {

  initscr();
  start_color();
  initNcursesPairs();
  attron(COLOR_PAIR(0));


  thread uiThread(ui);
	thread producerThread(producer, 10);
	thread consumerThread(consumer);

  uiThread.join();

  getch();
  endwin();
}
