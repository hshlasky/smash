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
#include <unistd.h>
//#include <algorithm>

#define MAX_JOBS 100
#define NO_PROCESS (-1)

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
public:
	int pid;
	bool stopped;
	time_t init_time;
	Command command;

	Process() : pid(0), stopped(true), init_time(0), command(Command()) {}
	Process(const int _pid, bool _stopped, Command _command)
		: pid(_pid), stopped(_stopped), command(std::move(_command)) {
		init_time = time(nullptr);
	}

	~Process() = default;

	void print_job() const {
		string stopped_text = stopped ? " (stopped)" : "";
		cout << command.to_string() << ": " << pid << " " << init_time << " " << stopped_text << endl;
	}

	int get_pid() const {
		return pid;
	}


};

class os {
	Process fg_process;	//pointer to the process in the foreground
	bool is_fg = false;		//tells if there is a process which runs foreground
	std::vector<Process> jobs_list;
	string last_wd;
	bool job_ids[MAX_JOBS] {true}; // An array of bools that represents the available pids.
	int max_job_id = NO_PROCESS;	//holds the highest id with a job occupied

public:
	os() : fg_process(Process()), last_wd(".") {
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

	void jobs() {
		for (int i=0 ; i<MAX_JOBS ; i++) {
			if (!job_ids[i]) {
				cout << "[" << i << "] ";
				jobs_list[i].print_job();
			}

		}
	}

	void fg_func(int job_id) {		//run the job in foreground
		if (!job_ids[job_id]) {		//checking if exist
			cout << "smash error: fg: job id " << job_id << " does not exist" << endl;
		} else {	//run in foreground, remove from the list
			fg_process = jobs_list[job_id];
			remove_job(job_id);
			is_fg = true;
			//run_command(fg_process.command);
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
		return is_fg ? fg_process.get_pid() : 0;
	}

	void kill_func(int signum, int job_id) {	//send signal number signum to process with job_d
		if (!job_ids[job_id]) {		//checking if that job exist
			cout << "job id " << job_id << " does not exist";
		}

		if (kill(signum, jobs_list[job_id]) == -1) {
			perror("smash error: kill failed");
			exit(1);
		}
		cout << "signal number " << signum << " was sent to pid " << job_id << endl;
		remove_job(job_id);
	}

	void bg_func(int job_id) {		//run in backround
		if (!job_ids[job_id]) {
			cout << "job id " << job_id << " does not exist";
		}
		if (!jobs_list[job_id].stopped) {
			cout << "smash error: bg: job id " << job_id << " is already in background";
		}

		if (kill(SIGCONT, jobs_list[job_id]) == -1) {
			perror("smash error: kill failed");
			exit(1);
		}
		jobs_list[job_id].stopped = true;
		jobs_list[job_id].print_job();
	}

	void bg_func() {		//run the job with max id in backround
		if (max_job_id == NO_PROCESS) {
			cout << "smash error: fg: jobs list is empty" << endl;
		} else {
			bg_func(max_job_id);
		}
	}

	void quit_func(bool got_kill) {		//kill smash, not completed
		if (got_kill) {
			for (int i = 0 ; i < MAX_JOBS ; i++) {
				if (job_ids[i]) {
					cout << "[" << job_ids[i] << "] ";
				}
			}
		}
		exit(0);
	}
};

os my_os;

void run_command(Command command) {	//for running in foreground
	switch (command.ord) {
		case showpid:
			showpid_func();
		break;
		case pwd:
			pwd_func();
		break;
		case cd:
			cd_func(command.args[1]);
		break;
		case jobs:
			my_os.jobs();
		case diff:
			diff_func(command.args[1], command.args[2]);
		break;
		case kill_o:
			my_os.kill_func(*command.args[1], *command.args[2]);
		break;
		case fg:		//we need to separate fg with arguments and without
			my_os.fg_func();
		break;
		case bg:		//we need to separate bg with arguments and without
			my_os.bg_func();
		break;
		case quit:		//we need to separate quit with kill and without
			my_os.quit_func(false);
		break;
		default:
			execv(command.args[0], command.args);
			// If the execv is successful, shouldn't get here.
			if (errno == ENOENT) // If the path doesn't exist
				cout << "smash error: external: cannot find program" << endl;
			else // Any other error.
				cout << "smash error: external: invalid command" << endl;
		break;
	}
}

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
	char _cmd[MAX_LINE_SIZE];

	//register signals handlers once at the start
	sig_reg();

	while(true) {
		printf("smash > ");
		fgets(command_line, MAX_LINE_SIZE, stdin);
		strcpy(_cmd, command_line);
		_cmd[strlen(command_line) + 1] = '\0';
		vector<Command> commands = get_commands(_cmd);

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

			if (command.is_bg) {
				pid_t pid = fork();
				if(pid < 0) {
					perror("fork fail");
					exit(1);
				}
				else if(pid > 0) {//father code
					my_os.new_job(getpid(), false, command);
					continue;
				}
				else
					if (setpgrp() < 0) {
						perror("setpgrp failed");
						exit(1);
					}
			}
			//for running in foreground
			run_command(command);
		}
		//initialize buffers for next command
		command_line[0] = '\0';
		_cmd[0] = '\0';
	}

	return 0;
}

#endif //SMASH_CPP_