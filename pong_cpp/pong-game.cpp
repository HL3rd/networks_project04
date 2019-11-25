#include <ncurses.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <mutex>

using namespace std;


int ballX, ballY;       // Position of ball
int dx, dy;             // Movement of ball
int padLY, padRY;       // Position of paddles
int scoreL, scoreR;     // Player scores

WINDOW *win;            // ncurses window

pthread_mutex_t boardItemsLock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {

    int refresh; // refresh is clock rate in microseconds
                // This corresponds to the movement speed of the ball



    return 0;
}
