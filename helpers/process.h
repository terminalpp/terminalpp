#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "helpers.h"

namespace helpers {

	/** Because different operating systems support different exit code types, we define the type exit code separately.
	 */
#ifdef _WIN64
	typedef unsigned long ExitCode;
#elif __linux__
	typedef int ExitCode;
#endif

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

		Command(std::vector<std::string> const& command) {
			auto i = command.begin(), e = command.end();
			command_ = *i;
			while (++i != e)
				args_.push_back(*i);
		}

		Command& operator = (Command const& other) {
			command_ = other.command_;
			args_ = other.args_;
			return *this;
		}
	private:
		std::string command_;
		std::vector<std::string> args_;
	};

	/** Represents access to the environment variables of a process. 
	 */
	class Environment {
	public:
		std::string const& operator [] (std::string const& name) const {
			auto i = map_.find(name);
			ASSERT(i != map_.end()) << "Environment value " << name << " not found";
			return i->second;
		}

		void unset(std::string const& name) {
			map_[name] = "";
	    }

		void unsetIfUnspecified(std::string const& name) {
			auto i = map_.find(name);
			if (i == map_.end()) {
				map_[name] = "";
			}
		}

		void set(std::string const& name, std::string const& value) {
			map_[name] = value;
		}

		void setIfUnspecified(std::string const& name, std::string const& value) {
			auto i = map_.find(name);
			if (i == map_.end()) {
				map_[name] = value;
			}
		}

		/** Applies the changes in the environment to the actual environment of the current process. 
		 */
		void apply() {
#ifdef _WIN64
			NOT_IMPLEMENTED;
#elif __linux__
			for (auto i : map_) {
				if (i.second.empty())
					unsetenv(i.first.c_str());
				else
					setenv(i.first.c_str(), i.second.c_str(), /* overwrite */ true); 
			}
#endif
		}

	private:
		std::unordered_map<std::string, std::string> map_;
	};


} // namespace helpers