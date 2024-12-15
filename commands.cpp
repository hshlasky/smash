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
#include <string>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>

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
	cout << "smash pid is " << getpid() << endl; // getpid() is always successful.
}

bool pwd_func()				//prints the current directory of smash
{
	char buffer[1024];
	if (!getcwd(buffer, sizeof(buffer))) {
		perror("smash error: getcwd failed");
		return false;
	}

	cout << buffer << endl;
	return true;
}

//assistance function for diff, check if the path is accessible (file or directory)
bool pathExists(const string& path)
{
	return access(path.c_str(), F_OK) != -1;
}

bool cd_func(const string& path)	//changes directory
{
	char temp[1024]; // Destination directory
	char cwd[1024];
	if (!getcwd(cwd, sizeof(cwd))) { // get cwd
		perror("smash error: getcwd failed");
		return false;
	}

	if (path == "-") { //change directory to the last path
		if (strlen(last_path) == 0) {
			cout << "smash error: cd: old pwd not set" << endl;
			return false;
		}
		strcpy(temp, last_path);
	}
	//back to the parent directory
	else if (path == ".."){
		size_t i;
		for (i = strlen(cwd) ; i > 0 ; i--) // find the last '/'
			if (cwd[i] == '/') break;

		if (i == 0) // No parent directory, do nothing
			return true;

		memcpy(temp, cwd, i);
		temp[i] = '\0';
	}
	//change directory to the given path
	else {
		if (!pathExists(path)) { // check if the given path exists
			cout << "smash error: cd: target directory does not exist" << endl;
			return false;
		}
		strcpy(temp, path.c_str());
	}

	if (chdir(temp) == -1) {
		perror("smash error: chdir failed");
		return false;
	}
	// Save cwd as last path for next call
	strcpy(last_path, cwd);
	return true;
}


//assistance function for diff, check if the path refers to a file
bool isFile(const string& path) {
	ifstream file(path.c_str());
	return file.good();  // If it can be opened, it's treated as a file
}
/* system call?
bool isFile(const string& path) {
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0) {
		return false; // Path does not exist
	}
	return S_ISREG(buffer.st_mode); // Check if it's a regular file
}*/

//assistance function for diff, compare files
//returns 0 if different and 1 if equal
//returns -1 on failure
int fileCompare(const string& path_1, const string& path_2) {
	ifstream file1(path_1.c_str(), ios::binary);
	ifstream file2(path_2.c_str(), ios::binary);

	//Check if both files opened successfully
	if (!file1.is_open() || !file2.is_open()) {
		perror("smash error: file open failed");
		return -1;  // Could not open one or both files
	}

	// Compare byte by byte
	return equal(istreambuf_iterator<char>(file1),
				  istreambuf_iterator<char>(),
				  istreambuf_iterator<char>(file2));
}

//comparing between files
bool diff_func(const string& path_1, const string& path_2)
{
	if (!pathExists(path_1) || !pathExists(path_2)) {
		cout << "smash error: diff: expected valid paths for files" << endl;
		return false;
	}

	if (!isFile(path_1) || !isFile(path_2)) {
		cout << "smash error: diff paths are not files" << endl;
		return false;
	}

	if (fileCompare(path_1, path_2)) {
		cout << "0" << endl;
	} else {
		cout << "1" << endl;
	}
	return true;
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
			invalid_args = num_args > 1 || (num_args == 1 && !is_number(args[1]));
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