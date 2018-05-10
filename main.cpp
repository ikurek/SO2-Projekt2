#include <iostream>
#include <thread>
#include <ncurses.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <string>

using namespace std;
mutex mtx;

// True if consumer is free
// False if it's taken
bool isConsumerEmpty = true;
// Variables that corespond to
// Sigle element of queue
condition_variable queueCV[3];
// Variable ther corresponds to
// Message inside consumer
condition_variable consumerCV;
// Counter for queue elements
// Maximuum is 3
int queue_count = 0;
// Counter for elements that didn't fit
// Queue and are returning
// Maximuum is 2
int round_count = 0;

// Prints '#' in given position
// With provided color
void printMessage(int x, int y, int color)
{
	attron(COLOR_PAIR(color));
	mvprintw(x, y, "#");
}

// Clears character in given position
void printEmpty(int x, int y)
{
	attron(COLOR_PAIR(7));
	mvprintw(x, y, " ");
}

// Repaints content of terminal
void refresh_display()
{
	while (true)
	{
		// Lock mutex and refresh curses
		mtx.lock();
		refresh();
		mtx.unlock();
		// Wait before next refresh
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

// Represents a thread for a single message
void messageThread(int id)
{
	// Get random colour from palette
	int color = (id % 6) + 1;
	// Number of element in queue
	// If not in queue, it's -1
	int queueNumber = -1;
	// Current state of element
	// 0 - Free
	// 1 - In consumer
	// 2 - Done
	int currentState = 0;

	// Loop thread to infinity
	while (true)
	{
		// At the baggining set current state to 0
		currentState = 0;

		// Generate message and print it
		// Moving to consumer
		for (int i = 0; i < 12; i++)
		{
			// Lock mutex and print single message
			mtx.lock();
			printMessage(7, i, color);
			mtx.unlock();
			// Make thread wait before drawing again
			this_thread::sleep_for(chrono::milliseconds(600));
			// Lock mutex and print empty after message
			mtx.lock();
			printEmpty(7, i);
			mtx.unlock();
		}

		// Iterate while message is free
		while (currentState == 0)
		{
			// If consumer is not working
			// Mark it as busy and element as consumed
			if (isConsumerEmpty)
			{
				mtx.lock();
				isConsumerEmpty = false;
				mtx.unlock();
				currentState = 1;
			}
			// In other case, check if queue has
			// Free space
			else if (queue_count < 3)
			{
				// Set number of element in queue
				// To the last free space in it
				queueNumber = queue_count;
				// Lock mutex and incrase queue counter
				mtx.lock();
				queue_count++;
				mtx.unlock();
				// Iterate and draw to the moment, when
				// Element leaves queue
				while (queueNumber > 0)
				{
					// Draw 2nd queue element
					if (queueNumber == 2)
					{
						mtx.lock();
						printMessage(4, 13, color);
						mtx.unlock();
					}
					// Draw 3rd queue element
					else if (queueNumber == 1)
					{
						mtx.lock();
						printEmpty(4, 13);
						printMessage(5, 13, color);
						mtx.unlock();
					}
					// Acquire lock on current queue element
					unique_lock<mutex> locker(mtx);
					// Wait until next queue element is free
					queueCV[queueNumber - 1].wait(locker);
					// Unlock current element
					locker.unlock();
					// Notify next queue element to move
					queueCV[queueNumber].notify_one();
					// Decrease count of queue elements
					queueNumber--;
				}

				// Draw first queue element
				mtx.lock();
				printEmpty(5, 13);
				printMessage(6, 13, color);
				mtx.unlock();

				// Acquire lock on current queue element
				unique_lock<mutex> locker(mtx);
				// Wait until consumer is free
				consumerCV.wait(locker);
				// Unlock mutex
				locker.unlock();
				// Notify next queue element
				queueCV[queueNumber].notify_one();

				// Lock mutex for queue
				mtx.lock();
				// Decrement count of queue elements
				queue_count--;
				// Clear space
				printEmpty(6, 13);
				// Mark consumer as busy
				isConsumerEmpty = false;
				// Unlock
				mtx.unlock();
				// Set current element currentState
				// To being in consumer
				currentState = 1;
			}
			// If element is not free
			// And it made less than 2 rounds around
			// Make it go again
			else if (round_count < 2)
			{
				// Lock mutex and increase
				// Number of rounds
				mtx.lock();
				round_count++;
				mtx.unlock();

				// Set starting point and move direction
				// 0 is up, 1 is right, 2 is down, 3 is left
				int y = 6;
				int x = 11;
				int currentMoveDirection = 0;

				// Simulate movement around queue
				while ((y != 7 || x != 11))
				{
					// Lock mutex and print message
					mtx.lock();
					printMessage(y, x, color);
					mtx.unlock();
					// Make thread wait
					std::this_thread::sleep_for(std::chrono::milliseconds(600));
					// Clear place where message was
					mtx.lock();
					printEmpty(y, x);
					mtx.unlock();
					// Change position depending on
					// Current move direction
					if (currentMoveDirection == 0)
					{
						y--;
					}
					else if (currentMoveDirection == 1)
					{
						x++;
					}
					else if (currentMoveDirection == 2)
					{
						y++;
					}
					else if (currentMoveDirection == 3)
					{
						x--;
					}
					// Change move direction depending on
					// Current move direction
					if ((x == 11 && y == 2) || (x == 17 && y == 2) || (x == 17 && y == 11))
					{
						currentMoveDirection++;
					}
					else if (x == 11 && y == 11)
					{
						currentMoveDirection = 0;
					}
				}
				// Lock mutex and decrease round count
				mtx.lock();
				round_count--;
				mtx.unlock();
			}
			// In other cases, set currentState of
			// message to done
			else
			{
				currentState = 2;
			}
		}
		// If message is in consumer
		if (currentState == 1)
		{
			// Lock mutex and print it inside
			mtx.lock();
			printMessage(7, 13, color);
			mtx.unlock();
			// Wait until consumer finishesh work
			std::this_thread::sleep_for(std::chrono::milliseconds(3600));
			// Lock mutex, mark consumer as free
			// And clear current position
			mtx.lock();
			isConsumerEmpty = true;
			printEmpty(5, 15);
			mtx.unlock();
			// Notify next queue element
			consumerCV.notify_one();
			// Set currentState to done
			currentState = 2;
		}
		// If message is done, make it wait for random time
		// Then set currentState to free and start again
		if (currentState == 2)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 6 + 1) * 600));
		}
	}
}

void initColours()
{
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_YELLOW, COLOR_BLACK);
	init_pair(4, COLOR_BLUE, COLOR_BLACK);
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6, COLOR_CYAN, COLOR_BLACK);
	init_pair(7, COLOR_WHITE, COLOR_BLACK);
	init_pair(8, COLOR_BLACK, COLOR_WHITE);
}

void printStaticUI() {

	// Print consumer
	attron(COLOR_PAIR(8));
	mvprintw(7, 12, "|");
	mvprintw(7, 14, "|");
	attron(COLOR_PAIR(7));

	mvprintw(6, 0, "-");
	mvprintw(6, 1, "-");
	mvprintw(6, 2, "-");
	mvprintw(6, 3, ">");

	mvprintw(3, 9, "^");
	mvprintw(4, 9, "|");
	mvprintw(5, 9, "|");

	mvprintw(1, 11, "-");
	mvprintw(1, 12, "-");
	mvprintw(1, 13, "-");
	mvprintw(1, 14, ">");

	mvprintw(5, 18, "|");
	mvprintw(6, 18, "|");
	mvprintw(7, 18, "v");

	mvprintw(12, 12, "<");
	mvprintw(12, 13, "-");
	mvprintw(12, 14, "-");
	mvprintw(12, 15, "-");

	mvprintw(9, 9, "^");
	mvprintw(10, 9, "|");
	mvprintw(11, 9, "|");


}

int main()
{
	initscr();
	start_color();
	curs_set(0);
	srand(time(NULL));
	initColours();

	printStaticUI();

	mtx.unlock();

	thread refr(refresh_display);

	thread *personThreads = new thread[10];
	for (int i = 0; i < 10; i++)
	{
		personThreads[i] = thread(messageThread, i);
		this_thread::sleep_for(std::chrono::milliseconds(1200));
	}

	refr.join();

	for (int i = 0; i < 10; i++)
	{
		personThreads[i].join();
	}
	delete[] personThreads;
	endwin();
	return 0;
}
