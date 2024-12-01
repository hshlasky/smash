// signals.cpp
#include "signals.h"
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include "smash.cpp"

using namespace std;

// Signal handler function for Ctrl+Z
void sigtstpHandler(int sig) {
    cout << "smash: caught CTRL+Z" << endl;
    if (my_os.fg_process) {
        kill(my_os.fg_process.pid , SIGSTOP);
        cout << "smash: proccess " << my_os.fg_process.pid << " was stopped" << endl;
    }
}

// Signal handler function for Ctrl+C
void sigintHandler(int sig) {
    cout << "caught CTRL+C" << endl;
    if (my_os.fg_process) {
      kill(my_os.fg_process.pid , SIGKILL);
      cout << "smash: proccess " << my_os.fg_process.pid << " was killed" << endl;
    }
}

//function that register signal handlers
void sig_reg() {
    //setting signal handler for CTRL+Z
    signal(SIGTSTP, sigtstpHandler);

    //setting signal handler for CTRL+C
    signal(SIGINT, sigintHandler);
}


