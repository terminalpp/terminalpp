#pragma once
#ifdef __linux___

#include "../terminal.h"

namespace vterm {
	 
	class PTYTerminal : public virtual IOTerminal {
	public:
		PTYTerminal(std::string const & command, std::initializer_list<std::string> args, unsigned cols, unsigned rows);

		~PTYTerminal() {

		}

		void execute();

	protected:

		bool readInputStream(char* buffer, size_t& size) override;

		void doResize(unsigned cols, unsigned rows) override;

		bool write(char const* buffer, size_t size) override;

	private:
		std::string command_;

		std::vector<std::string> args_;

		int pipe_;

		pid_t pid_;

	};
} // namespace vterm


#endif 
