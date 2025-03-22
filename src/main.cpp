#include <iostream>
#include <list>
#include <string>

#include "hash.h"
#include "log.h"
#include "uci.h"
#include "nnue.h"

int main(int argc, char* argv[]) {
    // IMPORTANT disable output buffering for both std::cout and printf
    std::cout.setf(std::ios::unitbuf);
    setvbuf(stdout, NULL, _IOLBF, 0);

    initLogging();
    InitZobrist();

    std::string argv_str(argv[0]);
    std::string base = argv_str.substr(0, argv_str.find_last_of("/"));
    
    // init_nnue(base + "/../weights/nnue_2025-02-27 17:18:02.625752.csv");

    UCI uci;

    std::string input_buffer;

    while (true) {
        // file descriptors for select
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        // timeout for how long select will wait
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 20000;

        int select_result = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);

        // i do not understand this
        if (select_result > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
            char buffer[1024];
            ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));

            if (bytes_read > 0) {
                input_buffer.append(buffer, bytes_read);

                size_t newline_pos;
                while ((newline_pos = input_buffer.find('\n')) != std::string::npos) {
                    std::string line = input_buffer.substr(0, newline_pos);
                    uci.writeTokenizedCommand(line);
                    input_buffer.erase(0, newline_pos + 1);
                }
            }
        } else {
            uci.consumeOutput();
        }
    }

    return 0;
}