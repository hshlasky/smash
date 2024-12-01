#ifndef SIGNALS_H_
#define SIGNALS_H_

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include "smash.cpp"

using namespace std;
/*=============================================================================
* global functions
=============================================================================*/
//function that register signal handlers
void sig_reg();

#endif //SIGNALS_H_