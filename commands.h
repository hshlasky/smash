#ifndef __COMMANDS_H__
#define __COMMANDS_H__
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <cassert>
#include <cstdlib> //for NULL
#include <string>
#include <utility>
#include <vector>
#include <map>

#define MAX_LINE_SIZE 80
#define MAX_ARGS 20
using namespace std;
enum order
{
	showpid,
	pwd,
	cd,
	jobs,
	kill,
	fg,
	bg,
	quit,
	diff,
	unknown
};


/*=============================================================================
* error definitions
=============================================================================*/
enum ParsingError
{
	VALID,
	INVALID_COMMAND,
	INVALID_ARGS,

	//feel free to add more values here
};

/*=============================================================================
* global functions
=============================================================================*/

class Command {
public:
	order ord;
	char* args[MAX_ARGS]{};
	char text[MAX_LINE_SIZE+1]; // char array that holds the whole command (+1 for '\0')
	int num_args;
	bool is_and;
	bool is_bg;

	explicit Command() : ord(unknown), num_args(0), is_and(false), is_bg(false) {}

	string to_string() const {
		string cmd = args[0];
		for (int i = 1 ; i <= num_args ; i++) {
			cmd += (" " + static_cast<string>(args[i]));
		}
		return cmd;
	}

	string get_args_error() const {
		string error_message = static_cast<string>(args[0]) + ": ";
		switch (ord) {
			case showpid:
			case pwd:
			case jobs:
				error_message += "expected 0 arguments";
			break;

			case cd:
				error_message += "expected 1 arguments";
			break;

			case kill:
			case fg:
			case bg:
				error_message += "invalid arguments";
			break;

			case quit:
				error_message += "unexpected arguments";
			break;

			case diff:
				error_message += "expected 2 arguments";
			break;

			default: // should not get here
				assert(0);
			break;
		}
		return error_message;
	}

	ParsingError parseCommand();
};

//prints the pid of the process of smash
void showpid_func();

//prints the current directory of smash
void pwd_func();

//changes directory
void cd_func(const char *path);

//compare content of folders
void diff_func(const char *path_1, const char *path_2);

#endif //__COMMANDS_H__