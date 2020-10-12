#pragma once
#if (defined ARCH_UNIX)
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif

#include <vector>
#include <unordered_map>
#include <memory>

#include "helpers.h"
#include "string.h"


HELPERS_NAMESPACE_BEGIN

	/** Because different operating systems support different exit code types, we define the type exit code separately.
	 */
#if (defined ARCH_WINDOWS)
	typedef unsigned long ExitCode;
#elif (defined ARCH_UNIX)
	typedef int ExitCode;
#else
#error "Unsupported platform"
#endif

	/** Encapsulation of a local command to be executed.

		Local command consists of the name (path) to the command (most likely an executable, but depending on the shell used can also be a shell command), list of its arguments and optionally its working directory. 
	 */
	class Command {
	public:
		std::string const& command() const {
			return command_;
		}

		void setCommand(std::string const& command) {
			command_ = command;
		}

        /** \name Working Directory
         
            The command may specify its working directory. If the working directory is empty (default value), then the working directory of the current process should be used. 
         */
        //@{

        std::string const & workingDirectory() const {
            return workingDirectory_;
        }

        Command & setWorkingDirectory(std::string const & value) {
            workingDirectory_ = value;
            return *this;
        }
        //@}

		std::vector<std::string> const& args() const {
			return args_;
		}

		Command & setArgs(std::vector<std::string> const& args) {
			args_ = args;
            return *this;
		}

		Command & setArgs(std::initializer_list<char const*> args) {
			args_.clear();
			for (auto i : args)
				args_.push_back(i);
            return *this;
		}

		/** Converts the command into a single string. 
		 */
		std::string toString() const {
			std::string result = command_;
			for (auto i : args_)
				result = result + ' ' + Quote(i);
			return result;
		}

		/** Returns the command as an argv array. 

		    If the array is not used in a fork() call, it must be disposed of properly, but its elements should not be deleted themselvs as they are pointers to the c string stored in the command itself. 

			The length of the array is number of arguments of the command + 2, where the first argument is the command itself, and the last argument is nullptr, as the standard requires. 
		 */
		char** toArgv() const {
			char** args = new char* [args_.size() + 2];
			args[0] = const_cast<char*>(command_.c_str());
			for (size_t i = 0; i < args_.size(); ++i)
				args[i + 1] = const_cast<char*>(args_[i].c_str());

			args[args_.size() + 1] = nullptr;
			return args;
		}

		/** Creates an empty command. 
		 */
		Command():
		    command_("") {
		}

		Command(std::string const& command, std::initializer_list<char const*> args) :
			command_(command) {
			setArgs(args);
		}

		Command(std::string const& command, std::initializer_list<char const*> args, std::string const & workingDirectory) :
			command_(command),
            workingDirectory_(workingDirectory) {
			setArgs(args);
		}

		Command(std::string const& command, std::vector<std::string> const& args) :
			command_(command),
			args_(args) {
		}

		Command(std::string const& command, std::vector<std::string> const& args, std::string const & workingDirectory) :
			command_{command},
			args_{args},
            workingDirectory_{workingDirectory} {
		}

		Command(std::vector<std::string> const& command) {
			auto i = command.begin(), e = command.end();
            if (i == e) {
                command_ = "";
            } else {
        		command_ = *i;
                while (++i != e)
                    args_.push_back(*i);
            }
		}

		Command(std::vector<std::string> const& command, std::string const & workingDirectory):
            Command{command} {
            workingDirectory_ = workingDirectory; 
    	}

		Command(Command const & from):
		    command_{from.command_},
			args_{from.args_},
            workingDirectory_{from.workingDirectory_} {
		}

		Command& operator = (Command const& other) {
			command_ = other.command_;
			args_ = other.args_;
            workingDirectory_ = other.workingDirectory_;
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
            if (! cmd.workingDirectory().empty())
                s << " [in " << cmd.workingDirectory() << "]";
			return s;
		}

		std::string command_;
		std::vector<std::string> args_;
        std::string workingDirectory_;
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
#if (defined ARCH_WINDOWS)
			NOT_IMPLEMENTED;
#else
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

		static char const * Get(std::string const & name) {
#if (defined ARCH_WINDOWS)
            MARK_AS_UNUSED(name);
			NOT_IMPLEMENTED;
#else
			return getenv(name.c_str());
#endif
		}

	private:
		std::unordered_map<std::string, std::string> map_;
	};

#ifdef ARCH_WINDOWS
    /** Global lock for creating processes on Windows. 
     
        To capture child process output a pipe has to be creates, allowing its handles to be inherited, then create the process swapping its std in/out and then closing our ends of the pipes. This means that when the child process closes its handles, the pipe will die and our ReadFile will be interrupted. 

        However if this process creation is interrupted by another process creation from different thread *after* the shared handle is created, but *before* it is closed, then both processes will inherit the handle and only when both processes die will the ReadFile be terminated. 

        Wrapping the problematic area in the CreateProcessGuard instance ensures that only one thread at a time can create these inherited handles, launch new process and close its own ends at a time.   
     */
    class CreateProcessGuard {
    public:
        CreateProcessGuard() {
            Mutex_().lock();
        }

        ~CreateProcessGuard() {
            Mutex_().unlock();
        }

    private: 
        static std::mutex & Mutex_() {
            static std::mutex m;
            return m;
        }

    }; // CreateProcessGuard

#endif


    /** Executes the given command and returns its output as a string. 
     
        Empty path means current directory.


    */
    inline std::string Exec(Command const& command, ExitCode* exitCode = nullptr) {
#ifdef ARCH_WINDOWS
        ExitCode ec = EXIT_FAILURE;
        std::string output;
        HANDLE pipeIn;
        HANDLE pipeTheirOut;
        HANDLE pipeOut;
        HANDLE pipeTheirIn;
        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
        {
            CreateProcessGuard g;
            // create the pipes
            SECURITY_ATTRIBUTES attrs;
            attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
            attrs.bInheritHandle = TRUE;
            attrs.lpSecurityDescriptor = NULL;
            // create the pipes
            if (!CreatePipe(& pipeIn, & pipeTheirOut, &attrs, 0) || !CreatePipe(& pipeTheirIn, & pipeOut, & attrs, 0))
                THROW(Exception()) << "Unable to open pipes for " << command;
            // make sure the pipes are not inherited
            if (!SetHandleInformation(pipeIn, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(pipeOut, HANDLE_FLAG_INHERIT, 0))
                THROW(Exception()) << "Unable to open pipes for " << command;
            // create the process
            STARTUPINFOA sInfo;
            ZeroMemory(&sInfo, sizeof(STARTUPINFOA));
            sInfo.cb = sizeof(STARTUPINFO);
            sInfo.hStdError = pipeTheirOut;
            sInfo.hStdOutput = pipeTheirOut;
            sInfo.hStdInput = pipeTheirIn;
            sInfo.dwFlags |= STARTF_USESTDHANDLES;
            std::string cmd = command.toString();
            OSCHECK(CreateProcessA(NULL,
                &cmd[0], // the command to execute
                NULL, // process security attributes 
                NULL, // primary thread security attributes 
                true, // handles are inherited 
                0, // creation flags 
                NULL, // use parent's environment 
                command.workingDirectory().empty() ? nullptr : command.workingDirectory().c_str(), // current directory
                &sInfo,  // startup info
                &pi)  // info about the process
            );
            // we can close our handles to the other ends now
            CloseHandle(pipeTheirOut);
            CloseHandle(pipeTheirIn);
        }
        // read the output
        std::stringstream result;
        char buffer[128];
        DWORD bytesRead;
        std::thread termination_monitor = std::thread([& pi, & ec](){
            OSCHECK(WaitForSingleObject(pi.hProcess, INFINITE) == 0);
            OSCHECK(GetExitCodeProcess(pi.hProcess, &ec) != 0);
        });
        //GetExitCodeProcess(pi.hProcess, &ec);
        while (ReadFile(pipeIn, &buffer, 128, &bytesRead, nullptr)) {
            if (bytesRead != 0)
                result << std::string(&buffer[0], &buffer[0] + bytesRead);
        }
        termination_monitor.join();
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(pipeIn);
        CloseHandle(pipeOut);
        //CloseHandle(pipeOut);
        // close the handles to created process & thread since we do not need them
        //CloseHandle(pi.hProcess);
        //CloseHandle(pi.hThread);
        output = result.str();
        // a crude detection whether the output is given in UTF16, or UTF8 (ASCII)
        if (output.size() > 1 && output[1] == 0) 
            output = UTF16toUTF8(std::wstring{reinterpret_cast<utf16_char const *>(output.c_str()), output.size() / 2}.c_str());
        if (exitCode != nullptr)
            * exitCode = ec;
        else if (ec != EXIT_SUCCESS)
            THROW(Exception()) << "Command " << command << " exited with code " << ec << ", output:\n" << output;
        return output;
#else
        // create the pipes
        int       toCmd[2];
        int       fromCmd[2];
        OSCHECK(pipe(toCmd) == 0 && pipe(fromCmd) == 0) << "Cannot create pipes for command " << command;
        // fork, create process and split between process and us (the reading)
        pid_t     pid;
        switch (pid = fork()) {
            case -1:
                OSCHECK(false) << "Cannot fork for command " << command;
                /* The child patches the pipe to the standatd input and output and then executes the command.
                */
            case 0: {
                // TODO perhaps no reason to do OSCHECK here since failure is in different process anyways? 
                OSCHECK(
                    dup2(toCmd[0], STDIN_FILENO) != -1 &&
                    dup2(fromCmd[1], STDOUT_FILENO) != -1 &&
                    dup2(fromCmd[1], STDERR_FILENO) != -1 &&
                    close(toCmd[1]) == 0 &&
                    close(fromCmd[0]) == 0
                ) << "Unable to change standard output for process " << command;
                // change directory only if the path is not empty (current dir)
                if (!command.workingDirectory().empty())
                    OSCHECK(chdir(command.workingDirectory().c_str()) == 0) << "Cannot change dir for command " << command;
                char** argv = command.toArgv();
                // execvp never returns
                OSCHECK(execvp(command.command().c_str(), argv) != -1) << "Unable to execute command" << command;
                UNREACHABLE;
                break;
            }
            // parent is after the switch
            default: 
                break;
        }
        /** If we are the parrent still, close our excess handles to the pipes and read the output.
         */
        OSCHECK(
            close(toCmd[0]) == 0 &&
            close(fromCmd[1]) == 0
        ) << "Unable to close pipes for command " << command;
        // now read the file
        std::stringstream result;
        char buffer[128];
        while (true) {
            int numBytes = ::read(fromCmd[0], buffer, 128);
            // end of file
            if (numBytes == 0)
                break;
            if (numBytes == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                OSCHECK(false) << "Cannot read output of command " << command;
            }
            // otherwise add the read data to the result
            result << std::string(buffer, numBytes);
        }
        // now get the exit code
        ExitCode ec;
        OSCHECK(waitpid(pid, &ec, 0) == pid);
        if (exitCode != nullptr)
            * exitCode = WEXITSTATUS(ec);
        else if (ec != EXIT_SUCCESS)
            THROW(Exception()) << "Command " << command << " exited with code " << ec << ", output:\n" << result.str();
        return result.str();
#endif
    }

HELPERS_NAMESPACE_END