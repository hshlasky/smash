//commands.cpp
#include <cstdlib>
#include "commands.h"
#include <string>
#include <cstring>
#include <iostream>

using namespace std;
//example function for parsing commands
bool is_number (char const* num) {
	return strtol(num, nullptr, 0);
}

void showpid_func() {
	cout << getpid() << endl;
}



ParsingError Command::parseCommand()
{
	const char* delimiters = " \t\n"; //parsing should be done by spaces, tabs or newlines
	char* command_text = strtok(text, delimiters); //read strtok documentation - parses string by delimiters
	if(!command_text)
		return INVALID_COMMAND; //this means no tokens were found, most like since command is invalid

	args[0] = command_text; //first token before spaces/tabs/newlines should be command name
	for(int i = 1; i < MAX_ARGS; i++)
	{
		args[i] = strtok(nullptr, delimiters); //first arg NULL -> keep tokenizing from previous call
		if(!args[i]) {
			char* last_arg = args[num_args];
			size_t len = strlen(last_arg);

			if (len > 0 && last_arg[len - 1] == '%') {
				is_bg = true; // Set background flag
				last_arg[len - 1] = '\0'; // Remove '%'

				// If the argument is now empty, remove it
				if (strlen(last_arg) == 0) {
					args[num_args] = nullptr; // Clear the empty argument
					num_args--; // Decrement argument count
				}
			}
			break;
		}

		num_args++;
	}

	bool invalid_args = false;
	switch (command_text) { // For each command, set the 'order' field and check argiment validity.
		case "showpid":
			order = showpid;
			invalid_args = num_args != 0;
			break;

		case "pwd":
			order = pwd;
			invalid_args = num_args != 0;
			break;

		case "cd":
			order = cd;
			invalid_args = num_args != 1;
			break;

		case "jobs":
			order = jobs;
			invalid_args = num_args != 0;
			break;

		case "kill":
			order = kill;
			// Check if the args are valid, should be: "kill -<signum> <job id>"
			invalid_args = num_args != 2 || args[1][0] != '-' || !is_number(args[1]) || !is_number(args[2]);
			break;

		// fg and bg have the same args format, so they are together.
		case "fg":
		case "bg":
			order = (command_text == "fg") ? fg : bg;
			// Check if the args are valid, should be: "fg/bg <job id>" (job id is optional)
			invalid_args = num_args > 1 || (num_args == 1 && !is_number(args[1]));
			break;

		case "quit":
			order = quit;
			// Check if the args are valid, should be: "quit [kill]" (kill is optional)
			invalid_args = num_args > 1 || (num_args == 1 && args[1] != "kill");
			break;

		case "diff":
			order = diff;
			invalid_args = num_args != 2;
			break;

		default:
			break;
	}

	if (invalid_args)
		return INVALID_ARGS;

	return VALID;
}

//example function for getting/setting return value of a process.
#include <unistd.h>
#include <sys/wait.h>
int processReturnValueExample()
{
	pid_t pid = fork();
	if(pid < 0)
	{
		perror("fork fail");
		exit(1);
	}
	else if(pid == 0) //child code
	{
		//do some work here - for example, execute an external command
		auto cmd = "/bin/ls";
		auto args[] = {"ls", "-l", NULL};
		execvp(cmd, args);
		//execvp never returns unless upon error
		perror("execvp fail");
		exit(1); //set nonzero exit code for father to read
	}
	else //father code
	{
		int status;
		waitpid(pid, &status, 0); //wait for child to finish and read exit code into status
		if(WIFEXITED(status)) //WIFEXITED determines if a child exited with exit()
		{
			int exitStatus = WEXITSTATUS(status);
			if(!exitStatus)
			{
				//exit status != 0, handle error	
			}
			else
			{
				//exit status == 0, handle success
			}
		}
	}

	return 0;
}

