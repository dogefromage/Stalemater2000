#include "log.h"

#include <stdlib.h>
#include <time.h>

FILE* logfile;

void initLogging() {
#ifdef LOG_FILE
    logfile = fopen(LOG_FILE, "a");
    if (logfile == NULL) {
        printf("Error opening the logfile.\n");
        exit(EXIT_FAILURE);
    }
    LOG("Starting log [pid=%d]\n", getpid());
#endif
}

std::string getTimeBuf() {
    time_t current_time = time(NULL);
    if (current_time == -1) {
        printf("Error getting the current time.\n");
        exit(EXIT_FAILURE);
    }
    struct tm* local_time = localtime(&current_time);
    if (local_time == NULL) {
        printf("Error converting to local time.\n");
        exit(EXIT_FAILURE);
    }
    std::string time(asctime(local_time));
    time.pop_back();
    return time;
}