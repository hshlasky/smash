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
//#include <algorithm>

#define MAX_JOBS 100
#define NO_PROCESS -1

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

	~Process() = default;

	void print_job() const {
		cout << command.to_string() << ": " << pid << " " << init_time << " " << (stopped) ? "(stopped)" : "" << endl;
	}

	int get_pid() const {
		return pid;
	}

	void run_command() {	//for running in foreground
		switch (command) {
			case showpid:
				showpid_func();
				break;
			case pwd:
				pwd_func();
				break;
			case cd:
				cd_func();
				break;
			case jobs:
				my_os->jobs();
			case diff:
				diff_func();
				break;
		}
	};
};

class os {
	Process *fg_process;	//pointer to the process in the foreground
	bool is_fg = false;		//tells if there is a process which runs foreground
	std::vector<Process> jobs_list;
	string last_wd;
	bool job_ids[MAX_JOBS] {true}; // An array of bools that represents the available pids.
	int max_job_id = NO_PROCESS;	//holds the highest id with a job occupied

public:
	os() : last_wd(".") {
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
		if (job_id > max_job_id) {
			max_job_id = job_id;
		}
	}

	// Function to remove a process from the list
	void remove_job(int job_id) {
		jobs_list.erase(jobs_list.begin() + job_id);
		job_ids[job_id] = false;

		//updating max_job_id
		if (job_id == max_job_id) {
			max_job_id = NO_PROCESS;
			for (int i = job_id ; i > NO_PROCESS ; i--) {
				if (job_ids[i]) max_job_id = i;
			}
		}
	}

	void fg_func(int job_id) {		//run the job in foreground
		if (!job_ids[job_id]) {		//checking if exist
			cout << "smash error: fg: job id " << job_id << " does not exist" << endl;
		} else {	//run in foreground, remove from the list
			fg_process = &jobs_list[job_id];
			remove_job(job_id);
			is_fg = true;
			fg_process->run_command();
			is_fg = false;
		}
	}

	void fg_func() {		//run the job with max id in foreground
		if (max_job_id == NO_PROCESS) {
			cout << "smash error: fg: jobs list is empty" << endl;
		} else {
			fg_func(max_job_id);
		}
	}

	int fg_exist() const {		//return if there is a process which runs foreground
		return is_fg;
	}

	int fg_pid() const {		//return foreground process pid
		return fg_process->get_pid();
	}

	void jobs_func() {		//prints all the jobs
		for (Process job : jobs_list) {
			job.print_job();
		}
	}

	void kill_func(int signum, int job_id) {	//not completed
		//(signum, jobs_list[job_id]);
		cout << "signal number " << signum  << " was sent to pid " << job_id << endl;
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