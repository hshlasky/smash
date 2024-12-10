//commands.cpp
#include <cstdlib>
#include "commands.h"
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <algorithm>

using namespace std;

//example function for parsing commands
bool is_number(const string& s) {
	if (s.empty())
		return false;
	if (s.size() == 1)
		return isdigit(s[0]); // Single character must be a digit
	bool first_char = isdigit(s[0]) || s[0] == '-'; // If the string conteins more than 1 char, first char can be '-'
	return first_char && all_of(s.begin()+1, s.end(), ::isdigit); // All other chars must be digits
}

// * * * orders definitions * * * //
char last_path[1024];		//remembers the last path that smash referred to

void showpid_func()			//prints the pid of the process of smash
{
	cout << getpid() << endl; // getpid() is always successful.
}

void pwd_func()				//prints the current directory of smash
{
	char buffer[1024];
	cout << getcwd(buffer, sizeof(buffer)) << endl;
}

void cd_func(const char *path)	//changes directory
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
		size_t i = 0;
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
	istringstream stream(text);
	string token;

	// Tokenize the input based on delimiters
	while (stream >> token) { // tokenize the text of the command by any whitespace (space, tab or newline).
		args.push_back(token);
		num_args++;
		if (num_args >= MAX_ARGS) {
			break; // Stop if maximum arguments are reached
		}
	}

	if (args.empty()) {
		return INVALID_COMMAND; // No tokens found, invalid command
	}

	// Check for background process marker '%'
	string &last_arg = args.back();
	if (!last_arg.empty() && last_arg.back() == '%') {
		is_bg = true;
		last_arg.pop_back(); // Remove '%'

		// Remove the argument if it is now empty
		if (last_arg.empty()) {
			args.pop_back();
			num_args--;
		}
	}

	bool invalid_args = false;
	unordered_map<string, function<void()>> commands = {
		{"showpid", [&]() {
			ord = showpid;
			invalid_args = num_args != 0;
		}},
		{"pwd", [&]() {
			ord = pwd;
			invalid_args = num_args != 0;
		}},
		{"cd", [&]() {
			ord = cd;
			invalid_args = num_args != 1;
		}},
		{"jobs", [&]() {
			ord = jobs;
			invalid_args = num_args != 0;
		}},
		{"kill", [&]() {
			ord = kill_o;
			// Check if the args are valid, should be: "kill -<signum> <job id>
			invalid_args = num_args != 2 || args[1][0] != '-' || !is_number(args[1]) || !is_number(args[2]);
		}},
		{"fg", [&]() {
			ord = fg;
			// Check if the args are valid, should be: "fg <job id>" (job id is optional)
			invalid_args = num_args > 1 || (num_args == 1 && !is_number(args[0]));
		}},
		{"bg", [&]() {
			ord = bg;
			// Check if the args are valid, should be: "bg <job id>" (job id is optional)
			invalid_args = num_args > 1 || (num_args == 1 && !is_number(args[1]));
		}},
		{"quit", [&]() {
			ord = quit;
			// Check if the args are valid, should be: "quit [kill]" (kill is optional)
			invalid_args = num_args > 1 || (num_args == 1 && args[1] != "kill");
		}},
		{"diff", [&]() {
			ord = diff;
			invalid_args = num_args != 2;
		}}
	};

	// Execute the appropriate command
	auto it = commands.find(args[0]);
	if (it != commands.end())
		it->second(); // Call the associated lambda

	if (invalid_args)
		return INVALID_ARGS;

	return VALID;
}

//example function for getting/setting return value of a process.
#include <unistd.h>
//#include <sys/wait.h>
/*int processReturnValueExample()
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
}*/

