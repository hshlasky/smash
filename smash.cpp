//smash.cpp

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include "commands.h"
#include "signals.h"
#include <vector>
#include <string>
#include <iostream>

enum process_state
{
FORGROUND,
BACKGROUND,
STOPPED,
WAITING
}



using namespace std;
/*=============================================================================
* classes/structs declarations
=============================================================================*/
class process {
	int pid;
	process_state state;
	int init_time;
	order my_order;
	process* next_process;
	vector<string> args;
}

class os {
	process* fg_process;
	std::vector<process> process_list;
	string last_wd;

}

os my_os;

/*=============================================================================
* global variables & data structures
=============================================================================*/
char _line[MAX_LINE_SIZE];

/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
	char _cmd[MAX_LINE_SIZE];
	while(1)
	{
		printf("smash > ");
		fgets(_line, MAX_LINE_SIZE, stdin);
		strcpy(_cmd, _line);
		_cmd[strlen(_line) + 1] = '\0';
		//execute command

		//initialize buffers for next command
		_line[0] = '\0';
		_cmd[0] = '\0';
	}

	return 0;
}
