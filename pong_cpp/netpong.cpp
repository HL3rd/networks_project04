#include <ncurses.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mutex>

using namespace std;

int ballX, ballY;       // Position of ball
int dx, dy;             // Movement of ball
int padLY, padRY;       // Position of paddles
int scoreL, scoreR;     // Player scores

WINDOW *win;            // ncurses window
bool ISHOST = false;    // global var to distinguish host player
int FINALSOCK;

pthread_mutex_t boardItemsLock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {

    int refresh; // refresh is clock rate in microseconds
                // This corresponds to the movement speed of the ball
    int sockFinal;

    if (argv[1] == "--host") {
        // set this connection as the host
    } else {
        // set this as the other player connecting
    }

    return 0;
}
