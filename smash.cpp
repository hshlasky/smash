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
#include <sys/wait.h>
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
	pid_t pid;
	bool stopped;
	time_t init_time;
	Command command;

	Process() : pid(0), stopped(true), init_time(0), command(Command()) {}
	Process(const pid_t _pid, bool _stopped, Command _command)
		: pid(_pid), stopped(_stopped), command(std::move(_command)) {
		init_time = time(nullptr);
		if (init_time == -1) {
			perror("smash error: time failed");
			exit(1);
		}
	}

	~Process() = default;

	void print_job() const {
		const string stopped_text = stopped ? " (stopped)" : "";
		const time_t current_time = time(nullptr);
		if (current_time == -1) {
			perror("smash error: time failed");
			exit(1);
		}
		cout << command.text << ": " << pid << " " << static_cast<int>(difftime(current_time, init_time)) << " secs" << stopped_text << endl;
	}

	inline int get_pid() const {
		return pid;
	}
};

class os {
	Process fg_process;	//pointer to the process in the foreground
	bool is_fg = false;		//tells if there is a process which runs foreground
	std::vector<Process> jobs_list;
	string last_wd;
	bool job_ids[MAX_JOBS+1] {false}; // An array of bools that represents the available pids (false = available)
	int max_job_id = NO_PROCESS;	//holds the highest id with a job occupied

public:
	os() : fg_process(Process()), last_wd(".") {
		jobs_list.resize(MAX_JOBS+1);
	}

	void set_fg(const bool fg) {
		is_fg = fg;
	}

	// Finds the lowest availale job id and mark it unavailable
	int allocate_job_id() {
		for (int i = 1 ; i <= MAX_JOBS ; i++) { // pid is between 1 and MAX_JOBS.
			if (!job_ids[i]) {
				job_ids[i] = true;
				return i;
			}
		}
		printf("Smash is full");
		exit(1);
	}
	int new_job(const pid_t pid, const bool stopped, const Command& cmd) {
		const int job_id = allocate_job_id();
		jobs_list[job_id] = Process(pid, stopped, cmd);
		if (job_id > max_job_id) {
			max_job_id = job_id;
		}
		return job_id;
	}

	int new_job(const Process& prcs, const bool stopped) {	//adding to list a fg process that was stoped
		const int job_id = allocate_job_id();
		jobs_list[job_id] = prcs;
		jobs_list[job_id].stopped = stopped;
		if (job_id > max_job_id) {
			max_job_id = job_id;
		}
		return job_id;
	}

	// Function to remove a process from the list
	void remove_job(const int job_id) {
		//jobs_list.erase(jobs_list.begin() + job_id);
		job_ids[job_id] = false;

		//updating max_job_id
		if (job_id == max_job_id) {
			max_job_id = NO_PROCESS;
			for (int i = job_id ; i > NO_PROCESS ; i--) {
				if (job_ids[i]) max_job_id = i;
			}
		}
	}

	int get_job_id(const pid_t pid) const {
		for (int i = 1; i <= max_job_id; i++) {
			if (job_ids[i] && jobs_list[i].get_pid() == pid)
				return i;
		}
		return -1;
	}

	void update_jobs_list() {
		pid_t pid;
		while ((pid = waitpid(-1, nullptr, WNOHANG)) > 0) {
			int job_id = get_job_id(pid);
			remove_job(job_id);
		}
	}

	void jobs() const{
		for (int i=1 ; i<=MAX_JOBS ; i++) {
			if (job_ids[i]) {
				cout << "[" << i << "] ";
				jobs_list[i].print_job();
			}

		}
	}
	int fg_pid() const {		//return foreground process pid
		return is_fg ? fg_process.get_pid() : 0;
	}
	bool fg_func(const int job_id, const bool is_bg) {		//run the job in foreground
		if (!job_ids[job_id]) {		//checking if exist
			cout << "smash error: fg: job id " << job_id << " does not exist" << endl;
			return false;
		}
		if (is_bg) {
			cout << "smash error: fg: cannot run in background" << endl;
			return false;
		}
		//run in foreground, remove from the list
		fg_process = jobs_list[job_id];
		remove_job(job_id);
		cout << "[" << job_id << "] " << fg_process.command.text << endl;
		is_fg = true;
		if (fg_process.stopped && kill(fg_pid(), SIGCONT) == -1) {
			perror("smash error: kill failed");
			return false;
		}
		fg_process.stopped = false;
		if (waitpid(fg_pid(), nullptr, 0) == -1) { // wait for the process to finish as it was in fg.
			perror("smash error: waitpid failed");
			return false;
		}
		is_fg = false;
		return true;
	}

	bool fg_func(bool is_bg) {		//run the job with max id in foreground
		if (max_job_id == NO_PROCESS) {
			cout << "smash error: fg: jobs list is empty" << endl;
			return false;
		}
		return fg_func(max_job_id, is_bg);
	}

	bool fg_no_prints(const int job_id, const bool is_bg) {		//run the job in foreground

		//run in foreground, remove from the list
		fg_process = jobs_list[job_id];
		remove_job(job_id);
		//cout << "[" << job_id << "] " << fg_process.command.text << endl;
		is_fg = true;
		if (fg_process.stopped && kill(fg_pid(), SIGCONT) == -1) {
			perror("smash error: kill failed");
			return false;
		}
		fg_process.stopped = false;
		if (waitpid(fg_pid(), nullptr, 0) == -1) { // wait for the process to finish as it was in fg.
			perror("smash error: waitpid failed");
			return false;
		}
		is_fg = false;
		return true;
	}

	int fg_exist() const {		//return if there is a process which runs foreground
		return is_fg;
	}

	bool kill_func(int signum, int job_id) {	//send signal number signum to process with job_d
		if (!job_ids[job_id]) {		//checking if that job exist
			cout << "job id " << job_id << " does not exist" << endl;;
			return false;
		}
		const pid_t pid = jobs_list[job_id].get_pid();
		if (kill(pid, signum) == -1) {
			perror("smash error: kill failed");
			return false;
		}
		cout << "signal " << signum << " was sent to pid " << pid << endl;
		return true;
	}

	bool bg_func(int job_id) {		//run in backround
		if (!job_ids[job_id]) {
			cout << "smash error: bg: job id " << job_id << " does not exist" << endl;
			return false;
		}
		Process job = jobs_list[job_id];
		if (!job.stopped) {
			cout << "smash error: bg: job id " << job_id << " is already in background" << endl;
			return false;
		}
		pid_t pid = job.get_pid();
		if (kill(pid ,SIGCONT) == -1) {
			perror("smash error: kill failed");
			return false;
		}
		jobs_list[job_id].stopped = false;
		cout  << fg_process.command.text << ": " << pid << endl;
		return true;
	}

	bool bg_func() {		//run the job with max id in backround
		if (max_job_id == NO_PROCESS) {
			cout << "smash error: bg: there are no stopped jobs to resume" << endl;
			return false;
		}
		return bg_func(max_job_id);
		/* Not sure if this is the idea, not fully clear in the assignment
		// Find the stopped proccess with the biggest job id
		int i;
		for (i=max_job_id ; i > NO_PROCESS ; i--) {
			if (jobs_list[i].stopped)
				break;
		}
		if (i == NO_PROCESS) {
			cout << "smash error: bg: there are no stopped jobs to resume" << endl;
			return false;
		}
		return bg_func(i);
		*/


	}

	// Quit smash and kill all its proccesses
	void quit_func() const {
		for (int i = 1 ; i <= max_job_id ; i++) {
			if (job_ids[i]) {
				Process p = jobs_list[i];
				pid_t pid = p.get_pid();
				cout << "[" << job_ids[i] << "] " << p.command.text << " - sending SIGTERM... ";
				if (kill(pid ,SIGTERM) == -1) {
					perror("smash error: kill failed");
					return;
				}
				int j;
				for (j=0; j < 5; j++) {
					sleep(1);
					pid_t result = waitpid(pid, nullptr, WNOHANG);
					if (result == pid)
						break;
					else if (result == -1) {
						perror("smash error: waitpid failed");
						return;
					}
				}
				if (j == 5) {
					cout << "sending SIGKILL ";
					if (kill(pid ,SIGKILL) == -1) {
						perror("smash error: kill failed");
						return;
					}
				}
				cout << "done" << endl;
			}
		}
		exit(0);
	}

	Process& get_fg_process() {
		return fg_process;
	}
};

os my_os;

bool run_command(const Command& command) {	//for running in foreground
	bool successful = true;
	pid_t pid;
	switch (command.ord) {
		case showpid:
			pid = command.is_bg ? getppid() : getpid();
			cout << pid << endl;
			//showpid_func();
		break;
		case pwd:
			successful = pwd_func();
		break;
		case cd:
			successful = cd_func(command.args[1]);
		break;
		case jobs:
			my_os.jobs();
		break;
		case diff:
			successful = diff_func(command.args[1], command.args[2]);
		break;
		case kill_o:
			successful = my_os.kill_func(-stoi(command.args[1]), stoi(command.args[2]));
		break;
		case fg:
			if (command.num_args == 1)
				successful = my_os.fg_func(stoi(command.args[1]), command.is_bg);
			else
				successful = my_os.fg_func(command.is_bg);
		break;
		case bg:
			if (command.num_args == 1)
				successful = my_os.bg_func(stoi(command.args[1]));
			else
				successful = my_os.bg_func();
		break;
		case quit:
			if (command.num_args == 0)
				exit(0);
			my_os.quit_func();
			return false; // If successful, won't arrive here
		break;
		default:
			pid = fork();
			if(pid < 0) {
				perror("fork fail");
				exit(1);
			}
			else if(pid > 0) {	//father code

				int job_id = my_os.new_job(pid, false, command);
				if (!command.is_bg)		//run as fg
					my_os.fg_no_prints(job_id, command.is_bg);

				int status;
				if (command.is_and || !command.is_bg) {
					if (wait(&status) == -1) { // wait for the child process to finish
						perror("smash error: wait failed");
						return false;
					}
					my_os.remove_job(job_id);
					if (status != 0)
						break;
				}
				if(WIFEXITED(status)) //WIFEXITED determines if a child exited with exit()
					successful = !WEXITSTATUS(status);
			}
			else { // child code
				if (setpgrp() < 0) {	//child runs setgrp before other codes
					perror("setpgrp failed");
					exit(1);
				}
				vector<const char*> args;
				for (const auto& arg : command.args) {
					args.push_back(arg.c_str());
				}
				args.push_back(nullptr); // Null-terminate the array

				// Use execv to execute the command
				execv(args[0], const_cast<char**>(args.data()));

				// If the execv is successful, shouldn't get here.
				if (errno == ENOENT) // If the path doesn't exist
					cout << "smash error: external: cannot find program" << endl;
				else // Any other error.
					cout << "smash error: external: invalid command" << endl;
				exit(1);
			}
		break;
	}
	return successful;
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
			cmd.text = command_line.substr(start, pos_and - start);
			cmd.is_and = true;
			start = pos_and + 2;
		}
		else if(pos_semicolon != string::npos) { // ";" was caught
			cmd.text = command_line.substr(start, pos_semicolon - start);
			start = pos_semicolon + 1;
		}
		else { // last command
			cmd.text = command_line.substr(start);
			reached_end = true;
		}
		commands.emplace_back(cmd);
	}
	return commands;
}

/*=============================================================================
* global variables & data structures
=============================================================================*/
// Signal handler function for Ctrl+Z
void sigtstpHandler(int sig) {
	cout << "smash: caught CTRL+Z" << endl;
	if (my_os.fg_exist()) {
		pid_t fg_pid = my_os.fg_pid();
		if (kill(fg_pid, SIGSTOP) == -1) {
			perror("smash error: kill failed");
		}
		cout << "smash: proccess " << fg_pid << " was stopped" << endl;
		my_os.new_job(my_os.get_fg_process(), true);
		my_os.set_fg(false);
	} else {
		return;
	}
}

// Signal handler function for Ctrl+C
void sigintHandler(int sig) {
	cout << "smash: caught CTRL+C\n" << endl;
	if (my_os.fg_exist()) {
		pid_t fg_pid = my_os.fg_pid();
		if (kill(fg_pid , SIGKILL) == -1) {
			perror("smash error: kill failed");
		}
		cout << "smash: proccess " << my_os.fg_pid() << " was killed" << endl;
		my_os.set_fg(false);
	} else {
		return;
	}
}

void sig_reg() {
	struct sigaction sa_tstp, sa_int;

	// Setting up the SIGTSTP (CTRL+Z) handler
	sa_tstp.sa_handler = sigtstpHandler;
	sigemptyset(&sa_tstp.sa_mask);
	sa_tstp.sa_flags = 0;
	sigaction(SIGTSTP, &sa_tstp, nullptr);

	// Setting up the SIGINT (CTRL+C) handler
	sa_int.sa_handler = sigintHandler;
	sigemptyset(&sa_int.sa_mask);
	sa_int.sa_flags = 0;
	sigaction(SIGINT, &sa_int, nullptr);
}


/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
	string command_line;
	//register signals handlers once at the start
	sig_reg();

	while(true) {
		cout << "smash > ";
		getline(cin, command_line);
		vector<Command> commands = get_commands(command_line);

		my_os.update_jobs_list();

		//execute command
		for (Command command : commands) {
			if (const ParsingError err = command.parseCommand()) {
				string error_message = "smash error: ";
				if (err == INVALID_COMMAND)
					error_message += "external: invalid command";
				else {
					error_message += command.get_args_error();
				}
				cout << error_message << endl;
				if (command.is_and) // The next commands should not execute.
					break;

				continue;
			}
			// At this point we have a parsed command with valid arguments.

			if (command.is_bg) {
					//if it's a built-in command, run a different process
				if (command.ord == showpid || command.ord == pwd || command.ord == cd
					|| command.ord == diff || command.ord == kill_o || command.ord == fg
					|| command.ord == bg || command.ord == quit || command.ord == jobs)
					{
					pid_t pid = fork();
					if(pid < 0) {
						perror("fork fail");
						exit(1);
					}
					else if(pid > 0) {//father code
						int job_id = my_os.new_job(pid, false, command);
						if (command.is_and) {
							int status;
							if (wait(&status) == -1) { // wait for the child process to finish
								perror("smash error: wait failed");
								break;
							}
							my_os.remove_job(job_id);
							if (status != 0)
								break;
						}
						continue;
					}
					//child runs setgrp before other codes
					if (setpgrp() < 0) {
						perror("setpgrp failed");
						exit(1);
					}
					exit(run_command(command));
				} else {	//if it's external command, the different process starts in the function
					bool successful = run_command(command);
					if (!successful && command.is_and) {
						break;
					}
				}
			}

			//for running in foreground
			if (!command.is_bg) {
				bool successful = run_command(command);
				if (!successful && command.is_and) {
					break;
				}
			}

		}
	}

	return 0;
}

#endif //SMASH_CPP_