#pragma once

#include <unordered_map>

#include "helpers/strings.h"

#include "terminal.h"

namespace vterm {

	class VT100 : public virtual PTYTerminal {
	public:
		VT100(unsigned cols, unsigned rows) :
			PTYTerminal(cols, rows) {
		}

	protected:

		void processInputStream(char * buffer, size_t & size) override;
	};



#ifdef FOOBAR


	/** Terminal connector that understands subset of VT100 commands.

	    Support at least the following escape sequences:

        https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

		There will most likely be some more to explore - check the ST terminal as well. 
	 */
	class VT100 : public VTerm {
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


			ShowCursor,
			HideCursor,

			Unknown,

		};

		VT100() :
			cursorCol_(0),
			cursorRow_(0),
			colorFg_(Color::White),
			colorBg_(Color::Black) {
		}


	protected:
		template<typename T>
		struct Argument {
			T value;
			bool specified;
		};

		/** Processes the bytes sent from the application.
		 */
		unsigned processBytes(char * buffer, unsigned size) override;

		/** Parses the escape sequence stored in buffer and advances the buffer pointer.

			If the escape sequence is incomplete and cannot be parsed, returns false.
		 */
		bool parseEscapeSequence(VirtualTerminal::ScreenBuffer & sb,char * & buffer, char * end);

		bool skipEscapeSequence(char * & buffer, char * end);

		Argument<int> parseNumber(char * & buffer, char * end, int defaultValue = 0);

		bool parseSetter(VirtualTerminal::ScreenBuffer & sb, char * & buffer, char * end);

		bool parseCSI(VirtualTerminal::ScreenBuffer & sb, char * & buffer, char * end);

		bool parseSpecial(VirtualTerminal::ScreenBuffer & sb, char * & buffer, char * end);



		void scrollLineUp(VirtualTerminal::ScreenBuffer & sb);

		void updateCursorPosition(VirtualTerminal::ScreenBuffer & sb);

		void print(VirtualTerminal::ScreenBuffer & sb, char const * text, unsigned length);

		void clear(VirtualTerminal::ScreenBuffer & sb, unsigned cols);

		void setCursor(VirtualTerminal::ScreenBuffer & sb,unsigned col, unsigned row);



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



#ifdef HAHA


		/** Initializes the escape sequences recognized by the terminal.
		 */
		static void InitializeEscapeSequences();

		class MatcherNode;
		class DefaultIntegerArgNode;
		class CheckIntegerArgNode;

		/** Escape sequence represents the successful result of matching the input for a VT escape sequence. 

		    It contains the kind of the escape code matched and values of any arguments the escape code had.

		 */
		class EscapeSequence {
		public:
			/** Internal representation of an escape sequence token.
			 */
			struct Token {
				enum class Kind {
					Character,
					Number,
					DefaultIntegerArg,
					CheckIntegerArg
				};
				Kind kind;
				union {
					char c;
					struct {
						unsigned index;
						int value;
					};
				};

				Token(char what) :
					kind(Kind::Character),
					c(what) {
				}
				Token(Kind kind, unsigned index, int value) :
					kind(kind),
					index(index),
					value(value) {
				}
			};

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
			friend class MatcherNode;
			friend class DefaultIntegerArgNode;
			friend class CheckIntegerArgNode;


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

			/** Adds new integer argument to the sequence being matched. 
			 */
			void addIntegerArgument(int value, bool specified) {
				args_.push_back(Argument(value, specified));
			}

			/** Arguments of the matched sequence. 
			 */
			std::vector<Argument> args_;

		};

		/** VT escape sequences matcher. 

		    VT sequences are matched using the matching FSM described by matcher nodes which match different parts of the escape sequences, and/or modify the result match. While not the fastest possible, the code should be simple enough to modify and update with new terminal sequences when/if they are required. 
		 */
		class Node {
		public:
			virtual bool match(char * buffer, char * end, EscapeSequence & seq) const = 0;
			virtual void addMatch(EscapeCode code, EscapeSequence::Token const * begin, EscapeSequence::Token const * end) = 0;

		protected:
			Node * createNode(EscapeCode code, EscapeSequence::Token const * t, VT100::EscapeSequence::Token const * end);
		};

		/** The terminal node terminates a valid escape sequence match. 

		    It is created at the end of the match and may have no children. 
		 */
		class TerminalNode : public Node {
		public:

			/** Terminal node always matches to its result. 
			 */
			bool match(char * buffer, char * end, EscapeSequence & seq) const override {
				seq.code = result_;
				seq.next = buffer;
				return true;
			}

			/** Adding a match to a terminal node is not possible, we only check that the code to match has been exhausted and that the requested escape sequence is the escape sequence returned.  because ambiguities may occur if terminal nodes are allowed children. 

			    NOTE: This is of course fixable, but at the moment I do not believe that VT sequences require this functionality. 
			 */
			void addMatch(EscapeCode code, EscapeSequence::Token const * begin, EscapeSequence::Token const * end) override {
				ASSERT(begin == end);
				ASSERT(code == result_);
			}

			TerminalNode(EscapeCode result) :
				result_(result) {
			}

		private:
			EscapeCode result_;

		};

		/** Matches either a single character, or a number, or both. 

		    This matcher class is a bit counter intuitive at first, because all numbers are optional. Therefore if a number is to me matched at the node, the number (or its default is matched first) and then the matcher node itself deals with the next character. 

			If number is not considered, then the next character match is performed immediately. 
		 */
		class MatcherNode : public Node {
		public:

			/** If number is to be matched, first matches the number (which may be empty) and adds the argument to the escape sequence result. Then matches on single character on which the transition to the next node is determined.  
			 */
			bool match(char * buffer, char * end, EscapeSequence & seq) const {
				if (matchNumber_) {
					int arg = 0;
					bool isSpecified = false;
					if (buffer != end) {
						while (helpers::IsDecimalDigit(*buffer)) {
							arg = arg * 10 + helpers::DecCharToNumber(*buffer++);
							isSpecified = true;
						}
					}
					seq.addIntegerArgument(arg, isSpecified);
				}
				// the matcher node expects at least one character 
				if (buffer == end)
					return false;
				// determine the transition and take it, or fail if transition not possible
				auto i = transitions_.find(*buffer++);
				if (i == transitions_.end())
					return false;
				return i->second->match(buffer, end, seq);
			}

			void addMatch(EscapeCode code, EscapeSequence::Token const * begin, EscapeSequence::Token const * end) override {
				switch (begin->kind) {
				// if current kind is number, make sure the node matches numbers and advance the token
				case EscapeSequence::Token::Kind::Number:
					matchNumber_ = true;
					++begin;
					ASSERT(begin != end);
					ASSERT(begin->kind == EscapeSequence::Token::Kind::Character) << "Character must be matched after number";
					// fallthrough
				case EscapeSequence::Token::Kind::Character: {
					auto i = transitions_.find(begin->c);
					if (i == transitions_.end())
						i = transitions_.insert(std::make_pair(begin->c, createNode(code, begin + 1, end))).first;
					return i->second->addMatch(code, begin + 1, end);
				}
				default:
					ASSERT(false) << "Matcher node only supports characters and numbers.";
				}
			}

			MatcherNode() :
				matchNumber_(false) {
			}

		private:
			bool matchNumber_;
			std::unordered_map<char, Node *> transitions_;
		};

		/** Provides given default value for an argument of specified index, if it was not specified by the user. 
		 */
		class DefaultIntegerArgNode : public Node {
		public:
			bool match(char * buffer, char * end, EscapeSequence & seq) const {
				ASSERT(index_ < seq.args_.size());
				EscapeSequence::Argument & arg = seq.args_[index_];
				ASSERT(arg.kind == EscapeSequence::Argument::Kind::Integer);
				if (!arg.specified)
					arg.valueInt = value_;
				next_->match(buffer, end, seq);
			}

			void addMatch(EscapeCode code, EscapeSequence::Token const * begin, EscapeSequence::Token const * end) override {
				NOT_IMPLEMENTED;
			}

			DefaultIntegerArgNode(unsigned index, int value) :
				index_(index),
				value_(value),
				next_(nullptr) {
			}

		private:

			unsigned index_;
			int value_;
			Node * next_;
		};

		class CheckIntegerArgNode : public Node {
		public:
			bool match(char * buffer, char * end, EscapeSequence & seq) const {
				ASSERT(index_ < seq.args_.size());
				EscapeSequence::Argument & arg = seq.args_[index_];
				ASSERT(arg.kind == EscapeSequence::Argument::Kind::Integer);
				auto i = transitions_.find(arg.valueInt);
				if (i == transitions_.end())
					return (defaultTransition_ == nullptr) ? false : defaultTransition_->match(buffer, end, seq);
				return i->second->match(buffer, end, seq);
			}

			void addMatch(EscapeCode code, EscapeSequence::Token const * begin, EscapeSequence::Token const * end) override {
				ASSERT(begin->kind == EscapeSequence::Token::Kind::CheckIntegerArg);
				ASSERT(begin->index == index_);
				auto i = transitions_.find(begin->value);
				if (i == transitions_.end())
					i = transitions_.insert(std::make_pair(begin->value, createNode(code, begin + 1, end))).first;
				return i->second->addMatch(code, ++begin, end);
			}

			CheckIntegerArgNode(unsigned index) :
				index_(index),
				defaultTransition_(nullptr) {
			}

		private:

			size_t index_;
			std::unordered_map<int, Node*> transitions_;
			Node * defaultTransition_;
		};


#if def HAHA

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
			std::unordered_map<int, Node *> transitions_;

			/** If true, the node is a number node. 
			 */
			bool isNumber_;
			
		}; 

#endif HA HA

		/** Adds an escape code definition to the matcher.
		 */
		static void AddEscapeCode(EscapeCode code, std::initializer_list<EscapeSequence::Token> match);


		/** Root node for matching the escape sequences. 
		 */
		static Node * root_;


#endif

	};
#ifdef HAHA
	/** Specialization for the integer argument getter of an escape sequence. 
	 */
	template<>
	inline int const & VT100::EscapeSequence::argument<int>(size_t index) const {
		ASSERT(index < args_.size());
		ASSERT(args_[index].kind == Argument::Kind::Integer);
		return args_[index].valueInt;
	}

#endif



#endif // FOOBAR

} // namespace vterm