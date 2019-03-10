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

			TODO refactor so that fields are private
		 */
		class EscapeSequence {
		public:
			EscapeCode code;
			char * next;
		private:
			friend class Node;
			void addIntegerArgument(int value, bool specified) {
				NOT_IMPLEMENTED;
			}

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


} // namespace vterm