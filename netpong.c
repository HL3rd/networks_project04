/* netpong.c */

/* * * * * * * * * * * * * * * *
 * Authors:
 *    Blake Trossen (btrossen)
 *    Horacio Lopez (hlopez1)
 * * * * * * * * * * * * * * * */

#include <ncurses.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils.h"

/* Define Macros */
#define streq(a, b) (strcmp(a, b) == 0)
#define WIDTH 43
#define HEIGHT 21
#define PADLX 1
#define PADRX WIDTH - 2

/* Define Globals */
// global variables recording the state of the game
int ballX, ballY;       // Position of ball
int dx, dy;             // Movement of ball
int padLY, padRY;       // Position of paddles
int scoreL, scoreR;     // Player scores
int roundNum;           // Round number
WINDOW *win;            // ncurses window

// other global variables
int is_host = 0;
FILE *client_file;
FILE *debug_file;

// need a lock to handle to lock active items
pthread_mutex_t boardLock = PTHREAD_MUTEX_INITIALIZER;

/* Define Game Functions */
/* Draw the current game state to the screen
 * ballX: X position of the ball
 * ballY: Y position of the ball
 * padLY: Y position of the left paddle
 * padRY: Y position of the right paddle
 * scoreL: Score of the left player
 * scoreR: Score of the right player
 */
void draw(int ballX, int ballY, int padLY, int padRY, int scoreL, int scoreR) {
    // Center line
    int y;
    for (y = 1; y < HEIGHT-1; y++) {
        mvwaddch(win, y, WIDTH / 2, ACS_VLINE);
    }
    // Score
    mvwprintw(win, 1, WIDTH / 2 - 3, "%2d", scoreL);
    mvwprintw(win, 1, WIDTH / 2 + 2, "%d", scoreR);
    // Ball
    mvwaddch(win, ballY, ballX, ACS_BLOCK);
    // Left paddle
    for (y = 1; y < HEIGHT - 1; y++) {
        int ch = (y >= padLY - 2 && y <= padLY + 2)? ACS_BLOCK : ' ';
        mvwaddch(win, y, PADLX, ch);
    }
    // Right paddle
    for (y = 1; y < HEIGHT - 1; y++) {
        int ch = (y >= padRY - 2 && y <= padRY + 2)? ACS_BLOCK : ' ';
        mvwaddch(win, y, PADRX, ch);
    }
    // Print the virtual window (win) to the screen
    wrefresh(win);
    // Finally erase ball for next time (allows ball to move before next refresh)
    mvwaddch(win, ballY, ballX, ' ');
}

/* Return ball and paddles to starting positions
 * Horizontal direction of the ball is randomized
 */
void reset() {
    ballX = WIDTH / 2;
    padLY = padRY = ballY = HEIGHT / 2;
    // dx is randomly either -1 or 1
    dx = (rand() % 2) * 2 - 1;
    dy = 0;
    // Draw to reset everything visually
    draw(ballX, ballY, padLY, padRY, scoreL, scoreR);
}

/* Display a message with a 3 second countdown
 * This method blocks for the duration of the countdown
 * message: The text to display during the countdown
 */
void countdown(const char *message) {
    int h = 4;
    int w = strlen(message) + 4;
    WINDOW *popup = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    box(popup, 0, 0);
    mvwprintw(popup, 1, 2, message);
    int countdown;
    for (countdown = 3; countdown > 0; countdown--) {
        mvwprintw(popup, 2, w / 2, "%d", countdown);
        wrefresh(popup);
        sleep(1);
    }
    wclear(popup);
    wrefresh(popup);
    delwin(popup);
    padLY = padRY = HEIGHT / 2; // Wipe out any input that accumulated during the delay
}

/* Display a message with the winner of the game
 * This method is activated when a side scores 2 points
 * message: 'WIN -->' or '<-- WIN'
 * Will concatenate message with Round # and newline
 */
void displayWinner(const char *winMessage) {
    int h = 4;
    int w = strlen(winMessage) + 4;
    WINDOW *popup = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    box(popup, 0, 0);
    char roundBuf[50];
    sprintf(roundBuf, "Round %d", roundNum);
    mvwprintw(popup, 1, 2, roundBuf);
    mvwprintw(popup, 2, 2, winMessage);
    int countdown;
    for (countdown = 3; countdown > 0; countdown--) {
        mvwprintw(popup, 3, w / 2, "%d", countdown);
        wrefresh(popup);
        sleep(1);
    }
    wclear(popup);
    wrefresh(popup);
    delwin(popup);
    scoreR = 0;                 // Clear scoreR
    scoreL = 0;                 // Clear scoreL
    roundNum += (roundNum + 1) % 100;   // Increment round number
    padLY = padRY = HEIGHT / 2; // Wipe out any input that accumulated during the delay
}

/* Perform periodic game functions:
 * 1. Move the ball
 * 2. Detect collisions
 * 3. Detect scored points and react accordingly
 * 4. Draw updated game state to the screen
 */
void tock() {
    // Move the ball
    ballX += dx;
    ballY += dy;

    // Check for paddle collisions
    // padY is y value of closest paddle to ball
    int padY = (ballX < WIDTH / 2) ? padLY : padRY;
    // colX is x value of ball for a paddle collision
    int colX = (ballX < WIDTH / 2) ? PADLX + 1 : PADRX - 1;
    if (ballX == colX && abs(ballY - padY) <= 2) {
        // Collision detected!
        dx *= -1;
        // Determine bounce angle
        if (ballY < padY) {
            dy = -1;
        } else if (ballY > padY) {
            dy = 1;
        } else {
            dy = 0;
        }
    }

    // Check for top/bottom boundary collisions
    if (ballY == 1) {
        dy = 1;
        // MISINg
    } else if (ballY == HEIGHT - 2) {
        dy = -1;
        // MISSING STUFF Here
    }

    // Score points
    if (ballX == 0) { // right side scores
        scoreR = (scoreR + 1) % 100;
        reset();
        if (scoreR == 2) {
            // Show winner
            // pthread_mutex_unlock(&boardLock);
            displayWinner("WIN -->");
            // pthread_mutex_lock(&boardLock);
        } else {
            // pthread_mutex_unlock(&boardLock);
            countdown("SCORE -->");
            // pthread_mutex_lock(&boardLock);
        }

    } else if (ballX == WIDTH - 1) { // left side scores
        scoreL = (scoreL + 1) % 100;
        reset();
        if (scoreL == 2) {
            // Show winner
            // pthread_mutex_unlock(&boardLock);
            displayWinner("<-- WIN");
            // pthread_mutex_lock(&boardLock);
        } else {
            // pthread_mutex_unlock(&boardLock);
            countdown("<-- SCORE");
            // pthread_mutex_lock(&boardLock);
        }
    }
    // Finally, redraw the current state
    draw(ballX, ballY, padLY, padRY, scoreL, scoreR);
}

void initNcurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    refresh();
    win = newwin(HEIGHT, WIDTH, (LINES - HEIGHT) / 2, (COLS - WIDTH) / 2);
    box(win, 0, 0);
    mvwaddch(win, 0, WIDTH / 2, ACS_TTEE);
    mvwaddch(win, HEIGHT-1, WIDTH / 2, ACS_BTEE);
}

/* Define Network Functions */
int open_socket_server(const char *port) {
    // get linked list of DNS results for corresponding host and port
    struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_INET;      // return IPv4 choices
    hints.ai_socktype   = SOCK_STREAM;  // use TCP (SOCK_DGRAM for UDP)
    hints.ai_flags      = AI_PASSIVE;   // use all interfaces

    struct addrinfo *results;
    int status;
    if ((status = getaddrinfo(NULL, port, &hints, &results)) != 0) {    // NULL indicates localhost
        fprintf(stderr, "%s:\terror:\tgetaddrinfo failed: %s\n", __FILE__, gai_strerror(status));
        return -1;
    }

    // iterate through results and attempt to allocate a socket, bind, and listen
    int server_fd = -1;
    struct addrinfo *p;
    for (p = results; p != NULL && server_fd < 0; p = p->ai_next) {
        // allocate the socket
        if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            fprintf(stderr, "%s:\terror:\tfailed to make socket: %s\n", __FILE__, strerror(errno));
            continue;
           }

        // bind the socket to the port
        if (bind(server_fd, p->ai_addr, p->ai_addrlen) < 0) {
            fprintf(stderr, "%s:\terror:\tfailed to bind: %s\n", __FILE__, strerror(errno));
            close(server_fd);
            server_fd = -1;
            continue;
        }

        // listen to the socket
        if (listen(server_fd, SOMAXCONN) < 0) {
            fprintf(stderr, "%s:\terror:\tfailed to listen: %s\n", __FILE__, strerror(errno));
            close(server_fd);
            server_fd = -1;
            continue;
        }
    }

    // free the linked list of address results
    freeaddrinfo(results);

    return server_fd;
}

FILE *accept_client(int server_fd) {
    struct sockaddr client_addr;
    socklen_t client_len = sizeof(struct sockaddr);

    // accept the incoming connection by creating a new socket for the client
    int client_fd = accept(server_fd, &client_addr, &client_len);
    if (client_fd < 0) {
        fprintf(stderr, "%s:\terror:\tfailed to accept client: %s\n", __FILE__, strerror(errno));
    }

    FILE *client_file = fdopen(client_fd, "w+");
    if (!client_file) {
        fprintf(stderr, "%s:\terror:\tfailed to fdopen: %s\n", __FILE__, strerror(errno));
        close(client_fd);
    }

    return client_file;
}

FILE *open_socket_client(char *host, char *port) {
	// get linked list of DNS results for corresponding host and port
    struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
    hints.ai_family     = PF_INET;      // return IPv4 and IPv6 choices
    hints.ai_socktype   = SOCK_STREAM;  // use TCP (SOCK_DGRAM for UDP)
    hints.ai_flags      = AI_PASSIVE;   // use all interfaces

	struct addrinfo *results;
    int status;
    if ((status = getaddrinfo(host, port, &hints, &results)) != 0) {    // NULL indicates localhost
        fprintf(stderr, "%s:\terror:\tgetaddrinfo failed: %s\n", __FILE__, gai_strerror(status));
        return NULL;
    }

    // iterate through results and attempt to allocate a socket and connect
    int client_fd = -1;
    struct addrinfo *p;
    for (p = results; p != NULL && client_fd < 0; p = p->ai_next) {
		// allocate the socket
        if ((client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            fprintf(stderr, "%s:\terror:\tunable to make socket: %s\n", __FILE__, strerror(errno));
            continue;
    	}

		// connect to the host
   		if (connect(client_fd, p->ai_addr, p->ai_addrlen) < 0) {
   		    close(client_fd);
   		    client_fd = -1;
   		    continue;
   		}
    }

    // free the linked list of address results
    freeaddrinfo(results);

    if (client_fd < 0) {
        fprintf(stderr, "%s:\terror:\tfailed to make socket to connect to %s:%s: %s\n", __FILE__, host, port, strerror(errno));
        return NULL;
    }

    /* Open file stream from socket file descriptor */
    FILE *client_file = fdopen(client_fd, "w+");
    if (client_file == NULL) {
        fprintf(stderr, "%s:\terror:\tfailed to fdopen: %s\n", __FILE__, strerror(errno));
        close(client_fd);
        return NULL;
    }

    return client_file;
}

void handler(int signal) {
    // send the termination message to the opponent
    fputs("EXIT\n", client_file); fflush(client_file);

    endwin();               // clean up ncurses
    fclose(client_file);    // close the client file

    exit(0);
}

/* Listen to keyboard input
 * Updates global pad positions
 */
void *listenInput(void *args) {
    while (1) {
        switch (getch()) {
            case KEY_UP:
                pthread_mutex_lock(&boardLock);
                if (is_host) {
                    padRY--;
                    fputs("PAD_R\n", client_file); fflush(client_file);
                    char new_y[BUFSIZ] = {0};
                    sprintf(new_y, "%d\n", padRY);
                    fputs(new_y, client_file); fflush(client_file);
                }
                pthread_mutex_unlock(&boardLock);
                break;
            case KEY_DOWN:
                pthread_mutex_lock(&boardLock);
                if (is_host) {
                    padRY++;
                    fputs("PAD_R\n", client_file); fflush(client_file);
                    char new_y[BUFSIZ] = {0};
                    sprintf(new_y, "%d\n", padRY);
                    fputs(new_y, client_file); fflush(client_file);
                }
                pthread_mutex_unlock(&boardLock);
                break;
            case 'w':
                pthread_mutex_lock(&boardLock);
                if (!is_host) {
                    padLY--;
                    fputs("PAD_L\n", client_file); fflush(client_file);
                    char new_y[BUFSIZ] = {0};
                    sprintf(new_y, "%d\n", padLY);
                    fputs(new_y, client_file); fflush(client_file);
                }
                pthread_mutex_unlock(&boardLock);
                break;
            case 's':
                pthread_mutex_lock(&boardLock);
                if (!is_host) {
                    padLY++;
                    fputs("PAD_L\n", client_file); fflush(client_file);
                    char new_y[BUFSIZ] = {0};
                    sprintf(new_y, "%d\n", padLY);
                    fputs(new_y, client_file); fflush(client_file);
                }
                pthread_mutex_unlock(&boardLock);
                break;
            default: break;
       }
    }
    return NULL;
}

void *listenNetwork(void *args) {
    // open a nonblocking stream for the client file
    int copy_client_fd = dup(fileno(client_file));
    int flags = fcntl(copy_client_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(copy_client_fd, F_SETFL, flags);
    FILE *client_file_nonblocking = fdopen(copy_client_fd, "w+");
    if (!client_file_nonblocking) {
        fprintf(stderr, "%s:\terror:\tfailed to fdopen: %s\n", __FILE__, strerror(errno));
        close(copy_client_fd);
        return NULL;
    }

    char message[BUFSIZ] = {0};
    while (1) {
        char *test = fgets(message, BUFSIZ, client_file_nonblocking);
        if (!test && errno == EWOULDBLOCK) {
            usleep(50);
            continue;
        }

        if (streq(message, "EXIT\n")) {
            endwin();               // clean up ncurses
            fclose(client_file);    // close the client file
            fclose(client_file_nonblocking);
            exit(0);
        }

        if (streq(message, "PAD_L\n")) {            // left paddle moves
            memset(message, 0, BUFSIZ);
            do {
                fgets(message, BUFSIZ, client_file);
            } while (strlen(message) == 0);
            rstrip(message);
            padLY = atoi(message);
        } else if (streq(message, "PAD_R\n")) {     // right paddle moves
            memset(message, 0, BUFSIZ);
            do {
                fgets(message, BUFSIZ, client_file);
            } while (strlen(message) == 0);
            rstrip(message);
            padRY = atoi(message);
        } else if (streq(message, "BALL\n")) {      // ball moves
            printf("%s", message);
        } else if (streq(message, "SCORE_L\n")) {   // update left-player's score
            printf("%s", message);
        } else if (streq(message, "SCORE_R\n")) {   // update right-player's score
            printf("%s", message);
        } else {
            fprintf(stderr, "%s:\terror:\treceived unknown message from opponent: %s", __FILE__, message);
        }
        //pthread_mutex_lock(&boardLock);
        memset(message, 0, BUFSIZ);
        //pthread_mutex_unlock(&boardLock);
    }
}

/* Main Execution */
int main(int argc, char *argv[]) {
    // process command line arguments
    if (argc != 3) {
		fprintf(stderr, "%s:\terror:\tincorrect number of arguments!\n", __FILE__);
        fprintf(stderr, "usage:\n");
		fprintf(stderr, "  host:       %s --host [port]\n", __FILE__);
        fprintf(stderr, "  challenger: %s [hostname] [port]\n", __FILE__);
		return EXIT_FAILURE;
	}

    char *host = NULL;
    if (streq(argv[1], "--host")) {
        is_host = 1;
    } else {
        host = argv[1];
    }

	char *port = argv[2];

    /***
    below is for debug purposes only
    ***/
    debug_file = fopen("debug_file.txt", "w+");

    int refresh;            // refresh is clock rate in microseconds, corresponds to the movement speed of the ball
    char difficulty[10];
    if (is_host) {
        // get refresh rate
        printf("Please select the difficulty level (easy, medium or hard): ");
        scanf("%s", &difficulty);
        if      (streq(difficulty, "easy"))    refresh = 80000;
        else if (streq(difficulty, "medium"))  refresh = 40000;
        else if (streq(difficulty, "hard"))    refresh = 20000;

        // open a socket
        int server_fd = open_socket_server(port);
        if (server_fd < 0) {
            fprintf(stderr, "%s:\terror:\tfailed to open server file\n", __FILE__);
            return EXIT_FAILURE;
        }

        do {
            // accept incoming client connection
            client_file = accept_client(server_fd);
        } while (!client_file);

        // wait for a challenger to establish a game session
        char buffer[BUFSIZ] = {0};
        do {
            fgets(buffer, BUFSIZ, client_file);
            if (streq(buffer, "CHALLENGE EXTENDED\n")) { break; }
            fprintf(stderr, "%s:\terror:\tunexpected message received: %s\n", __FILE__, buffer);
        } while (1);

        fputs("CHALLENGE ACCEPTED\n", client_file); fflush(client_file);
        memset(buffer, 0, BUFSIZ);
        strcat(buffer, difficulty);
        strcat(buffer, "\n");
        fputs(buffer, client_file); fflush(client_file);
    } else {
        // connect to host
        client_file = open_socket_client(host, port);
        if (!client_file) {
            fprintf(stderr, "%s:\terror:\tfailed to open server file\n", __FILE__);
            return EXIT_FAILURE;
        }

        fputs("CHALLENGE EXTENDED\n", client_file); fflush(client_file);
        char buffer[BUFSIZ] = {0};
        fgets(buffer, BUFSIZ, client_file);
        if (!streq(buffer, "CHALLENGE ACCEPTED\n")) {
            fprintf(stderr, "%s:\terror:\thost rejected request with response: %s\n", __FILE__, buffer);
            return EXIT_FAILURE;
        }

        // get the difficulty level
        memset(buffer, 0, BUFSIZ);
        fgets(buffer, BUFSIZ, client_file);
        rstrip(buffer);
        if      (streq(buffer, "easy"))     refresh = 80000;
        else if (streq(buffer, "medium"))   refresh = 40000;
        else if (streq(buffer, "hard"))     refresh = 20000;
        else {
            fprintf(stderr, "%s:\terror:\treceived invalid difficulty level from host: %s\n", __FILE__, buffer);
            return EXIT_FAILURE;
        }

    }

    // Set up signal handler
    signal(SIGINT, handler);

    // Set up ncurses environment
    initNcurses();

    // Set starting game state and display a countdown
    reset();
    countdown("Starting Game");

    // Listen to keyboard input in a background thread
    pthread_t pth;
    pthread_create(&pth, NULL, listenInput, NULL);

    // Start the network thread to listen to the opponent
    pthread_t pth_network;
    pthread_create(&pth_network, NULL, listenNetwork, NULL);

    // Main game loop executes tock() method every REFRESH microseconds
    struct timeval tv;
    while (1) {
        gettimeofday(&tv,NULL);
        unsigned long before = 1000000 * tv.tv_sec + tv.tv_usec;
        pthread_mutex_lock(&boardLock);
        tock(); // Update game state
        pthread_mutex_unlock(&boardLock);
        gettimeofday(&tv,NULL);
        unsigned long after = 1000000 * tv.tv_sec + tv.tv_usec;
        unsigned long toSleep = refresh - (after - before);
        // toSleep can sometimes be > refresh, e.g. countdown() is called during tock()
        // In that case it's MUCH bigger because of overflow!
        if (toSleep > refresh) toSleep = refresh;
        usleep(toSleep); // Sleep exactly as much as is necessary
    }

    // Clean up
    pthread_join(pth, NULL);
    endwin();
    return 0;
}
