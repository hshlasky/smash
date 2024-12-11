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
#include <numeric>

#define MAX_LINE_SIZE 80
#define MAX_ARGS 20
using namespace std;
enum order
{
	showpid,
	pwd,
	cd,
	jobs,
	kill_o,
	fg,
	bg,
	quit,
	diff,
	unknown
};

using namespace std;

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
	vector<string> args; // A vector of strings holding the args
	string text;  // The original text of the command
	int num_args;
	//bool is_and;
	bool is_bg;

	explicit Command() : ord(unknown), num_args(-1)/*, is_and(false)*/, is_bg(false) {}

	string to_string() const {
		return accumulate(args.begin() + 1, args.end(), args[0],
			[](const string& a, const string& b) {
				return a + " " + b;
			});
	}

	string get_args_error() const {
		string error_message = args[0] + ": ";
		switch (ord) {
			case showpid:
			case pwd:
			case jobs:
				error_message += "expected 0 arguments";
			break;

			case cd:
				error_message += "expected 1 arguments";
			break;

			case kill_o:
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
bool pwd_func();

//changes directory
bool cd_func(const string& path);

//compare content of folders
bool diff_func(const string& path_1, const string& path_2);

vector<string> split_string(const string& text, char delimiter = ' ');
#endif //__COMMANDS_H__