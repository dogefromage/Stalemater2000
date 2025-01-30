#pragma once
#include <unistd.h>

#include <cstdio>
#include <string>

#define LOG_FILE "/home/seb/git/Stalemater2000/bin/log.log"

#ifdef LOG_FILE

#define LOG(...)                           \
    fprintf(logfile, "%s ", getTimeBuf().c_str()); \
    fprintf(logfile, __VA_ARGS__);         \
    fflush(logfile)
#else
#define LOG(...) (void)

#endif

extern FILE* logfile;

void initLogging();
std::string getTimeBuf();