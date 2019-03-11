#pragma once

#include <unordered_map>

#include "vterm_connector.h"

namespace vterm {

	/** Terminal connector that understands subset of VT100 commands.

	    Support at least the following escape sequences:

        https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

		There will most likely be some more to explore - check the ST terminal as well. 
	 */
	class VT100 : public VirtualTerminal::Connector {
	public:
		enum class EscapeCode {
			CursorUp,
			CursorDown,
			CursorForward,
			CursorBackward,

			EraseCharacter,
			/** Erase n next characters in the viewport. 
			 */
			EraseInDisplay,

			Unknown,

		};

		VT100() :
			cursorCol_(0),
			cursorRow_(0),
			colorFg_(Color::White),
			colorBg_(Color::Black) {
			InitializeEscapeSequences();
		}


	protected:

		/** Processes the bytes sent from the application.
		 */
		unsigned processBytes(char * buffer, unsigned size) override;

		char * skipUnknownEscapeSequence(char * buffer, char * end);

		void scrollLineUp(VirtualTerminal::ScreenBuffer & sb);

		void updateCursorPosition(VirtualTerminal::ScreenBuffer & sb);

		void print(VirtualTerminal::ScreenBuffer & sb, char const * text, unsigned length);

		void clear(VirtualTerminal::ScreenBuffer & sb, unsigned cols);




		/** Current position of the cursor. 
		 */
		unsigned cursorCol_;
		unsigned cursorRow_;

		/** Background and foreground color to be used for new text. 
		 */
		Color colorFg_;
		Color colorBg_;

		/** Font to be used for new text. 
		 */
		Font font_;




		/** Initializes the escape sequences recognized by the terminal.
		 */
		static void InitializeEscapeSequences();

		class Node;

		/** Escape sequence represents the successful result of matching the input for a VT escape sequence. 

		    It contains the kind of the escape code matched and values of any arguments the escape code had.

		 */
		class EscapeSequence {
		public:
			/** The matched escape code if match was successful. 

			    EscapeCode::Unknown if unsuccessful;
			 */
			EscapeCode code;

			/** The first character from the buffer after the matched sequence. 

			    nullptr if the match was not successful. 
			 */
			char * next;

			/** Return the number of the arguments provided (either by user, or by default values) for the escape sequence. 
			 */
			size_t numArgs() const {
				return args_.size();
			}

			/** Returns true if given argument was specified by the user, false if the value it contains is the default value for the given escape sequence. 
			 */
			bool isArgumentSpecified(size_t index) const {
				ASSERT(index < args_.size());
				return args_[index].specified;
			}

			/** Assuming the index-th argument is integer, returns its value. 
			 */
			template<typename T>
			T const & argument(size_t index) const;

		private:
			friend class Node;

			/** Record for a given argument. 

			    TODO add string argument.
			 */
			struct Argument {
				/** Kind of the argument. 
				 */
				enum class Kind {
					Integer,
				};
				
				/** Kind of the actual argument. 
				 */
				Kind kind;

				/** True if the argument was specified, false if its value is the default value for the matched sequence. 
				 */
				bool specified;

				/** The value of the argument, based on the kind. 
				 */
				union {
					int valueInt;
				};

				/** Creates new integer value. 
				 */
				Argument(int value, bool specified) :
					kind(Kind::Integer),
					specified(specified),
					valueInt(value) {
				}

			};

			void addIntegerArgument(int value, bool specified) {
				args_.push_back(Argument(value, specified));
			}

			std::vector<Argument> args_;

		};

		/** 
		 */
		class Node {
		public:

			Node() :
				result_(EscapeCode::Unknown),
				isNumber_(false) {
			}

			bool match(char * buffer, char * end, EscapeSequence & seq);

			void addMatch(EscapeCode code, char const * begin, char const * end);




		private:
			/** If the node is terminator, contains the matched escape code.
			 */
			EscapeCode result_;
			
			/** Contains transitions to next matching nodes. 
			 */
			std::unordered_map<char, Node *> transitions_;

			/** If true, the node is a number node. 
			 */
			bool isNumber_;
			
		};

		/** Adds an escape code definition to the matcher.
		 */
		static void AddEscapeCode(EscapeCode code, std::initializer_list<char> match);


		/** Root node for matching the escape sequences. 
		 */
		static Node * root_;

	};

	/** Specialization for the integer argument getter of an escape sequence. 
	 */
	template<>
	inline int const & VT100::EscapeSequence::argument<int>(size_t index) const {
		ASSERT(index < args_.size());
		ASSERT(args_[index].kind == Argument::Kind::Integer);
		return args_[index].valueInt;
	}



} // namespace vterm