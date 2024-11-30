// signals.cpp
#include "signals.h"
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>

// Signal handler function for SIGTSTP (Ctrl+Z)
void sigtstpHandler(int sig) {
    //sigstp for the front process
    std::cout << "caught CTRL+Z" << std::endl;
}

int main() {
    // Set up the signal handler for SIGTSTP
    struct sigaction sa;
    sa.sa_handler = &sigtstpHandler;  // Set the handler function
    sigemptyset(&sa.sa_mask);         // No signals are blocked during the handler execution
    sa.sa_flags = 0;                  // No special flags

    // Register the handler for SIGTSTP (Ctrl+Z)
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        std::cerr << "Error setting up signal handler" << std::endl;
        return 1;
    }

    // Infinite loop to keep the program running and receiving signals
    while (true) {
        std::cout << "Waiting for SIGTSTP (Ctrl+Z)..." << std::endl;
        sleep(5);  // Sleep for 5 seconds between prints
    }

    return 0;
}

