#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "helpers.h"

namespace helpers {

	/** Because different operating systems support different exit code types, we define the type exit code separately.
	 */
#ifdef _WIN64
	typedef unsigned long ExitCode;
#elif (defined __linux__) || (defined __APPLE__)
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
		 */
		std::string toString() const {
			std::string result = command_;
			for (auto i : args_)
				result = result + ' ' + Quote(i);
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

		/** Quotes the given string for shell purposes. 

		    Space and quotes are quoted, everything else stays as is. If there are no characters that need to be quoted in the string, returns the unchanged argument, otherwise returns the quoted argument in double quotes. 
		 */
		static std::string Quote(std::string const& arg) {
			std::stringstream result;
			bool quoted = false;
			for (size_t i = 0, e = arg.size(); i != e; ++i) {
				switch (arg[i]) {
					case ' ':
						quoted = true;
						result << ' ';
						break;
					case '"':
					case '\'':
						quoted = true;
						result << '\\' << arg[i];
						break;
					default:
						result << arg[i];
				}
			}
			if (quoted) {
				result << '"';
				return std::string("\"") + result.str();
			} else {
				return result.str();
			}
		}

	private:

		friend std::ostream& operator << (std::ostream& s, Command const& cmd) {
			s << cmd.toString();
			return s;
		}

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
#elif (defined __linux__) || (defined __APPLE__)
			for (auto i : map_) {
				if (i.second.empty())
					unsetenv(i.first.c_str());
				else
					setenv(i.first.c_str(), i.second.c_str(), /* overwrite */ true); 
			}
#endif
		}

		/** Creates an empty environment. 
		 */
		Environment() = default;

		/** Creates an environment from given string map. 
		 */
		Environment(std::unordered_map<std::string, std::string> const& from) {
			for (auto i : from)
				map_[i.first] = i.second;
		}

	private:
		std::unordered_map<std::string, std::string> map_;
	};

	/** Executes the given command and returns its output as a string. 
	 */
	inline std::string Exec(Command const& command, std::string const & path, helpers::ExitCode * exitCode = nullptr) {
#ifdef _WIN64
		// create the pipes
		SECURITY_ATTRIBUTES attrs;
		attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
		attrs.bInheritHandle = TRUE;
		attrs.lpSecurityDescriptor = NULL;
		Win32Handle pipeIn;
		Win32Handle pipeOut;
		Win32Handle pipeTheirIn;
		Win32Handle pipeTheirOut;
		// create the pipes
		if (!CreatePipe(pipeTheirIn, pipeOut, &attrs, 0) || !CreatePipe(pipeIn, pipeTheirOut, &attrs, 0))
			THROW(Exception()) << "Unable to open pipes for " << command;
		// make sure the pipes are not inherited
		if (!SetHandleInformation(pipeIn, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(pipeOut, HANDLE_FLAG_INHERIT, 0))
			THROW(Exception()) << "Unable to open pipes for " << command;
		// create the process
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		STARTUPINFOA sInfo;
		ZeroMemory(&sInfo, sizeof(STARTUPINFOA));
		sInfo.cb = sizeof(STARTUPINFO);
		sInfo.hStdError = pipeTheirOut;
		sInfo.hStdOutput = pipeTheirOut;
		sInfo.hStdInput = pipeTheirIn;
		sInfo.dwFlags |= STARTF_USESTDHANDLES;
		std::string cmd = command.toString();
		if (!CreateProcessA(NULL,
			&cmd[0], // the command to execute
			NULL, // process security attributes 
			NULL, // primary thread security attributes 
			true, // handles are inherited 
			0, // creation flags 
			NULL, // use parent's environment 
			path.c_str(), // current directory
			&sInfo,  // startup info
			&pi)  // info about the process
			)
			THROW(Exception()) << "Unable to create process for " << command;
		// we can close our handles to the other ends now
		pipeTheirOut.close();
		pipeTheirIn.close();
		// read the output
		std::stringstream result;
		char buffer[128];
		DWORD bytesRead;
		while (ReadFile(pipeIn, & buffer, 128, &bytesRead, nullptr)) {
			if (bytesRead != 0)
				result << std::string(&buffer[0], &buffer[0] + bytesRead);
		}
		helpers::ExitCode ec;
		GetExitCodeProcess(pi.hProcess, &ec);
		// close the handles to created process & thread since we do not need them
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (exitCode != nullptr) 
			*exitCode = ec;
		else if (ec != EXIT_SUCCESS)
			THROW(Exception()) << "Command " << command << " exited with code " << ec << ", output:\n" << result.str();
		return result.str();
#else
		// TODO does not return exit code now!!!!
		char buffer[128];
		std::string result = "";
		std::string cmd = STR("cd \"" << path << "\" && " << command << " 2>&1");
		std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
		if (not pipe)
			throw std::runtime_error(STR("Unable to execute command " << cmd));
		while (not feof(pipe.get())) {
			if (fgets(buffer, 128, pipe.get()) != nullptr)
				result += buffer;
		}
		return result;
#endif

	}


} // namespace helpers