#ifndef __SIGNALS_H__
#define __SIGNALS_H__

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
void sig_reg();


#endif //__SIGNALS_H__