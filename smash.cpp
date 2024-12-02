//smash.cpp
#ifndef SMASH_CPP_
#define SMASH_CPP_
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <cassert>
#include <cstdlib>
#include "commands.h"
#include "signals.h"
#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>

#define MAX_JOBS 100
enum process_state
{
	FOREGROUND,
	BACKGROUND,
	STOPPED,
	WAITING
};



using namespace std;
/*=============================================================================
* classes/structs declarations
=============================================================================*/
class Process{
	int pid;
	bool stopped;
	time_t init_time;
	Command command;

public:
	Process(const int _pid, bool _stopped, Command _command)
		: pid(_pid), stopped(_stopped), command(std::move(_command)) {
		init_time = time(nullptr);
	}

	void print_job() const {
		cout << command.to_string() << ": " << pid << " " << init_time << " " << (stopped) ? "(stopped)" : "" << endl;
	}

	int get_pid() const {
		return pid;
	}
};

class os {
	int fg_process;	//the place of the process in the foreground
	std::vector<Process> jobs_list;
	string last_wd;
	bool job_ids[MAX_JOBS] {true}; // An array of bools that represents the available pids.

public:
	os() : fg_process(-1), last_wd(".") {
		jobs_list.resize(MAX_JOBS);
	}

	int find_lowest_available_job_id() {
		for (int i = 0 ; i < MAX_JOBS ; i++) {
			if (job_ids[i]) {
				job_ids[i] = false;
				return i + 1; // pid is between 1 and MAX_JOBS.
			}
		}
		printf("Smash is full");
		exit(1);
	}
	void new_job(int pid, bool stopped, const Command& cmd) {
		int job_id = find_lowest_available_job_id();
		jobs_list[job_id] = Process(pid, stopped, cmd);
	}

	int fg_pid() const {		//function to get the foreground pid
		return jobs_list[fg_process].get_pid();
	}

};

os my_os;



// Splits a command line string into separate commands based on the delimiters "&&" and ";".
// Each command is stored in a vector.
// The function updates the feilds "text" with the command text and "is_and" that is true if the delimiter was "&&".
//
// If the delimiter is "&&", it means subsequent commands should not execute if this command fails.
// If the delimiter is ";", it allows execution of the next command regardless of the result.
vector<Command> get_commands(const string& command_line) {
	vector<Command> commands;
	size_t start = 0;
	bool reached_end = false;
	while (!reached_end) {
		Command cmd;
		size_t pos_and = command_line.find("&&", start);
		size_t pos_semicolon = command_line.find(';', start);
		if (pos_and < pos_semicolon && pos_and != string::npos) { // "&&" was caught
			strcpy(cmd.text, command_line.substr(start, pos_and - start).c_str());
			cmd.is_and = true;
			start = pos_and + 2;
		}
		else if(pos_semicolon != string::npos) { // ";" was caught
			strcpy(cmd.text,command_line.substr(start, pos_semicolon - start).c_str());
			start = pos_semicolon + 1;
		}
		else { // last command
			strcpy(cmd.text, command_line.substr(start).c_str());
			reached_end = true;
		}
		commands.emplace_back(cmd);
	}
	return commands;
}

/*=============================================================================
* global variables & data structures
=============================================================================*/
char command_line[MAX_LINE_SIZE];

void pwd_func() {
	cout << getcwd(/*add parameters*/) << endl;
}

/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
	//char _cmd[MAX_LINE_SIZE];

	//register signals handlers once at the start
	sig_reg();

	while(true)
	{
		printf("smash > ");
		fgets(command_line, MAX_LINE_SIZE, stdin);
		vector<Command> commands = get_commands(command_line);
		//strcpy(_cmd, command_line);
		//_cmd[strlen(command_line) + 1] = '\0';
		//execute command
		for (Command command : commands) {
			if (const ParsingError err = command.parseCommand()) {
				string error_message = "error: ";
				if (err == INVALID_COMMAND)
					error_message = "external: invalid command";
				else {
					error_message += command.get_args_error();
				}
				if (command.is_and) // The next commands should not execute.
					break;
				else
					continue;
			}
			// At this point we have a parsed command with valid arguments.

		}


		//initialize buffers for next command
		command_line[0] = '\0';
		//_cmd[0] = '\0';
	}

	return 0;
}

#endif //SMASH_CPP_