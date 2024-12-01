// signals.cpp
#include "signals.h"
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

// Signal handler function for Ctrl+Z
void sigtstpHandler(int sig) {
    //sigstp for the front process
    cout << "caught CTRL+Z" << endl;
}

// Signal handler function for Ctrl+C
void sigintHandler(int sig) {
    cout << "caught CTRL+C" << endl;
}

//function that register signal handlers
void sig_reg() {
    //setting signal handler for CTRL+Z
    signal(SIGTSTP, sigtstpHandler);

    //setting signal handler for CTRL+C
    signal(SIGINT, sigintHandler);
}


