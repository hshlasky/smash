//commands.cpp
#include <cstdlib>
#include "commands.h"
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fstream>

using namespace std;
//example function for parsing commands
bool is_number (char const* num) {
	return strtol(num, nullptr, 0);
}


// * * * orders definitions * * * //
char last_path[1024];

void showpid_func()
{
	cout << getpid() << endl;
}

void pwd_func()
{
	char buffer[1024];
	cout << getcwd(buffer, sizeof(buffer)) << endl;
}

void cd_func(const char *path)
{
	char temp[1024];

	//change directory to the last path
	if (strcmp(path, "-") == 0) {
		getcwd(temp, sizeof(temp));
		chdir(last_path);				//last_path is a global variable in commands.cpp
		strcpy(last_path, temp);
	}
	//back to the parent directory
	else if (strcmp(path, "..") == 0){
		getcwd(last_path, sizeof(last_path));

		int slash_num = 0;
		int i = 0;
		while (slash_num < 1 && i < strlen(last_path)) {
			if (last_path[i] == '/') slash_num++;
			else if (last_path[i] == ' ') break;
			i++;
		}
		if (slash_num > 1) {
			memcpy(temp, last_path, i);
			chdir(temp);
		}
	}
	//change directory to the given path
	else {
		getcwd(last_path, sizeof(last_path));
		chdir(path);
	}
}

//assistance function for diff, check if the path is accessible (file or directory)
bool pathExists(const string& path)
{
	return access(path.c_str(), F_OK) != -1;
}

//assistance function for diff, check if the path refers to a file
bool isFile(const string& path)
{
	ifstream file(path);  // Try to open the file
	return file.good();        // Return true if the file is opened successfully
}

//assistance function for diff, compare files
bool fileCompare(const char *path_1, const char *path_2) {
	ifstream file1(path_1, ios::binary);
	ifstream file2(path_2, ios::binary);

	//Check if both files opened successfully
	if (!file1.is_open() || !file2.is_open()) {
		return false;  // Could not open one or both files
	}

	// Compare byte by byte
	return equal(istreambuf_iterator<char>(file1),
				  istreambuf_iterator<char>(),
				  istreambuf_iterator<char>(file2));
}

//comparing between files
void diff_func(const char *path_1, const char *path_2)
{
	if (!pathExists(path_1) || !pathExists(path_2)) {
		cout << "smash error: diff: expected valid paths for files" << endl;
	} else {
		if (!isFile(path_1) || !isFile(path_2)) {
			cout << "smash error: diff paths are not files" << endl;
		} else {
			if (fileCompare(path_1, path_2)) {
				cout << "0" << endl;
			} else {
				cout << "1" << endl;
			}
		}
	}
}


// * * * errors * * * //
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
			ord = showpid;
			invalid_args = num_args != 0;
			break;

		case "pwd":
			ord = pwd;
			invalid_args = num_args != 0;
			break;

		case "cd":
			ord = cd;
			invalid_args = num_args != 1;
			break;

		case "jobs":
			ord = jobs;
			invalid_args = num_args != 0;
			break;

		case "kill":
			ord = kill;
			// Check if the args are valid, should be: "kill -<signum> <job id>"
			invalid_args = num_args != 2 || args[1][0] != '-' || !is_number(args[1]) || !is_number(args[2]);
			break;

		// fg and bg have the same args format, so they are together.
		case "fg":
		case "bg":
			ord = (command_text == "fg") ? fg : bg;
			// Check if the args are valid, should be: "fg/bg <job id>" (job id is optional)
			invalid_args = num_args > 1 || (num_args == 1 && !is_number(args[1]));
			break;

		case "quit":
			ord = quit;
			// Check if the args are valid, should be: "quit [kill]" (kill is optional)
			invalid_args = num_args > 1 || (num_args == 1 && args[1] != "kill");
			break;

		case "diff":
			ord = diff;
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

