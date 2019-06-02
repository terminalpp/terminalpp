#pragma once

#include <string>
#include <vector>

namespace helpers {

	/** Encapsulation of a local command to be executed.

		Local command consists of the name (path) to the command (most likely an executable, but depending on the shell used can also be a shell command) and a vector of its arguments.
	 */
	class Command {
	public:
		std::string const& command() const {
			return command_;
		}

		void setCommand(std::string const& command) {
			command_ = command;
		}

		std::vector<std::string> const& args() const {
			return args_;
		}

		void setArgs(std::vector<std::string> const& args) {
			args_ = args;
		}

		void setArgs(std::initializer_list<char const*> args) {
			args_.clear();
			for (auto i : args)
				args_.push_back(i);
		}

		/** Converts the command into a single string. 

		    TODO quote arguments where necessary, etc. 
		 */
		std::string toString() const {
			std::string result = command_;
			for (auto i : args_)
				result = result + ' ' + i;
			return result;
		}

		Command(std::string const& command, std::initializer_list<char const*> args) :
			command_(command) {
			setArgs(args);
		}

		Command(std::string const& command, std::vector<std::string> const& args) :
			command_(command),
			args_(args) {
		}
	private:
		std::string command_;
		std::vector<std::string> args_;
	};


} // namespace helpers