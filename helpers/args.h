#pragma once

#include <vector>
#include <unordered_map>

#include "helpers.h"
#include "string.h"

/* What we need to express:

   - arguments can have aliases
   - arguments can be set via '=' or via spaces 
   - last argument
   - boolean arguments with default values simply set negate themselves when present
   - positional arguments
   


 */
namespace helpers {

	/** Exceptiomn thrown when invalid arguments are passed to the application. 
	 */
	class ArgumentError : public Exception {
	};

	/** Common base for command line arguments. 
	 */
	class BaseArg {
	public:

		static constexpr int NOT_POSITIONAL = -1;

		/** Returns the name of the argument. 
		 */
		std::string const& name() const {
			return name_;
		}

		/** Returns the aliases of the arguments, i.e. prefixes after which the argument is specified. 
		 */
		std::unordered_set<std::string> const& aliases() const {
			return aliases_;
		}

		/** Returns the description of the argument, which can be displayed in help screens and so on. 
		 */
		std::string const& description() const {
			return description_;
		}

		/** Returns true if the argument must be provided by the user. 
		 */
		bool required() const {
			return required_;
		}

		/** Returns true if the argument has been provided by the user. 
		 */
		bool specified() const {
			return specified_;
		}

	protected:

		friend class Arguments;

		/** Creates an argument. 

		    An argument has the following properties:

			name - used to determine the argument name when refering to it
			aliases - prefixes which enable setting the value of the argument on the commandline
			position - either NOT_POSITIONAL if the argument does not have fixed position, or the position at which the argument is to be found in the command line.
			required - if true, the argument must be provided, if false it may be skipped. All positional arguments but the last one must be required.
			last - if the argument is last, any other arguments after it are not parsed as separate arguments, but as additional values to the argument.
			description - help

			TODO make this nicer
		 */
		BaseArg(std::string const & name, std::initializer_list<char const*> aliases, int position, bool required, bool last, std::string const& description);

		/** Called by the arguments managed with the value intended for the argument. 

		    This method must be implemented by BaseArg descendants and the appropriate value must be parsed and the value updated. An exception should be thrown in case of any errors. 

			The default implementation stub simply throws if the value has already been specified before.
		 */
		virtual void parse(char const* value) {
			MARK_AS_UNUSED(value);
			if (specified_)
				THROW(ArgumentError()) << "Argument " << name_ << " already specified.";
		}

		/** Determines whether the argument expects value, or whether the mere presence of the argument on the commandline is enough for its value specification. 

		    This has no meaning if the value is given using the '=' character. 
		 */
		virtual bool expectsValue() const = 0;

		std::string name_;
		std::unordered_set<std::string> aliases_;
		std::string description_;
		bool required_;
		bool last_;
		bool specified_;
	}; // helpers::BaseArg

	/** Command line argument of various types. 

	 */
	template<typename T>
	class Arg : public BaseArg {
	public:
		Arg(std::string const & name, std::initializer_list<char const*> aliases, T const& value, bool required = false, std::string const& description = "", bool isLast = false, int position = BaseArg::NOT_POSITIONAL) :
			BaseArg(name, aliases, position, required, isLast, description),
			value_(value) {
		}

		/** Dereferencing the argument produces its value. 
		 */
		T const & operator * () const {
			return value_;
		}

		T const* operator ->() const {
			return &value_;
		}

	protected:

		/** This must be overriden by the specifications. 
		 */
		void parse(char const* value) override;

		/** By default, value is expected for all arguments. 
		
		    Types which do not require the value may specialize the method for themselves. 
		 */
		bool expectsValue() const override {
			return true;
		}

		T value_;
	}; // helpers::Arg<T>

	/** Aggregates all arguments passed on the command line and orchestrates the parsing of the commandline. 

	    
	 */
	class Arguments {
	public:

		/** Returns the command line used to run the application.

		    This is the first argument given to the app. 
		 */
		static std::string const& CommandLine() {
			return ArgumentsList().commandLine;
		}

		/** Parses given command line expressed as utf8 string. 
		 */
		static void Parse(char const * arguments) {

		}

		/** Parses the given commandline. 
		 */
		static void Parse(int argc, char* argv[]) {
			int i = 0;
			try {
				Impl& x = ArgumentsList();
				// verify that all positional arguments are specified and there are no holes
				for (size_t j = 0, je = x.byPosition.size(); j < je; ++j)
					ASSERT(x.byPosition[j] != nullptr) << "Unspecified argument position " << j;
				// check that we at least have the command line arguments
				if (argc == 0)
					THROW(ArgumentError()) << "Invalid number of arguments: 0";
				// set the command line
				x.commandLine = argv[0];
				// now analyze the command line by parsing the arguments
				i = 1;
				ParsePositionalArguments(x, i, argc, argv);
				ParseNamedArguments(x, i, argc, argv);
			} catch (ArgumentError const& e) {
				// TODO and do some error ofc
				std::cerr << e << std::endl;
				Help(std::cerr);
				std::exit(-1);
			}
		}

		/** Prints the usage help. 
		 */
		static void Help(std::ostream& s) {
			s << "This is help, but there is not much to see here yet. TODO";
		}

	private:
		friend class BaseArg;

		class Impl {
		public:
			std::string commandLine;
			std::unordered_map<std::string, BaseArg*> byAlias;
			std::unordered_map<std::string, BaseArg*> byName;
			std::vector<BaseArg*> byPosition;
		}; // Arguments::Impl

		static Impl& ArgumentsList() {
			static Impl impl;
			return impl;
		}

		static void ParsePositionalArguments(Impl & args, int& i, int argc, char* argv[]) {
			// if there are no positional arguments, don't do anything
			if (args.byPosition.empty())
				return;
			for (BaseArg* arg : args.byPosition) {
				// if there are no more arguments, throw an error if the current argument is required, otherwise continue to the next one - just a precaution, there should be no next one as only the last positional argument can be optional
				if (i == argc) {
					if (arg->required_)
					    THROW(ArgumentError()) << "Expected value for " << arg->name() << " but end of arguments found";
					continue;
				}
			    // parse the argument and move to the next one
				arg->parse(argv[i]);
				arg->specified_ = true;
				++i;
				// if the current argument is last, parse all remaining arguments in it as well
				if (arg->last_) {
					while (i < argc) {
						arg->parse(argv[i]);
						++i;
					}
				}
			}
		}

		static void ParseNamedArguments(Impl& args, int& i, int argc, char* argv[]) {
			while (i < argc) {
				char* argValue = nullptr;
				// assume the whole argument is the argument name
				std::string argName(argv[i]);
				auto arg = args.byAlias.find(argName);
				// if no such alias exist, search for the '=' character that would split the argName and value
				if (arg == args.byAlias.end()) {
					for (int j = 0; argv[i][j] != 0; ++j)
						if (argv[i][j] == '=') {
							argValue = argv[i] + j + 1;
							argName = std::string(argv[i], j);
							break;
						}
					arg = args.byAlias.find(argName);
					if (arg == args.byAlias.end())
						THROW(ArgumentError()) << "Unrecognized argument " << argName;
				} 
				// ok, we now have argument and its name. If value is nullptr, we must determine if value is next argument or not
				if (argValue == nullptr && arg->second->expectsValue()) {
					if (++i == argc)
						THROW(ArgumentError()) << "Expected value for argument " << arg->second->name();
					argValue = argv[i];
				}
				// now parse the argument and move to the next
				arg->second->parse(argValue);
				arg->second->specified_ = true;
				++i;
				// if the current argument is last, parse all remaining arguments in it as well
				if (arg->second->last_) {
					while (i < argc) {
						arg->second->parse(argv[i]);
						++i;
					}
				}
			}
			// now all arguments have been parsed, we must check that all required arguments are properly specified
			for (auto arg : args.byName)
				if (arg.second->required_ && !arg.second->specified_)
					THROW(ArgumentError()) << "Argument " << arg.first << " required, but value not specified";
		}

	}; // helpers::Arguments

	inline BaseArg::BaseArg(std::string const& name, std::initializer_list<char const*> aliases, int position, bool required, bool last, std::string const& description) :
	    name_(name),
		aliases_(),
		description_(description),
		required_(required),
		last_(last),
		specified_(false) {
		// register with the arguments by name
		Arguments::Impl & x = Arguments::ArgumentsList();
		ASSERT(x.byName.find(name_) == x.byName.end()) << "Argument named " << name_ << " already defined";
		x.byName.insert(std::make_pair(name_, this));
		// fill the aliases array and register with the arguments by alias
		for (char const* a : aliases) {
			std::string alias(a);
			ASSERT(aliases_.find(alias) == aliases_.end()) << "Argument " << name_ << " defines alias " << alias << " twice";
			aliases_.insert(alias);
			ASSERT(x.byAlias.find(alias) == x.byAlias.end()) << "Argument " << name_ << " uses already registered alias " << alias;
			x.byAlias.insert(std::make_pair(alias, this));
		}
		// if the argument is positional, add it to the positional vectors and make sure the index matches
		if (position != NOT_POSITIONAL) {
			ASSERT(required || last) << "All but last positional arguments must be required for argument " << name;
			if (static_cast<size_t>(position) >= x.byPosition.size())
				x.byPosition.resize(position + 1, nullptr);
			ASSERT(x.byPosition[position] == nullptr) << "Expected position " << position << " already taken by argument " << x.byPosition[position]->name() <<  " for argument " << name;
			ASSERT(position == 0 || x.byPosition[position - 1]->last_ == false) << "Only last positional argument can be designated as last argument (argument " << name << ")";
			x.byPosition[position] = this;
		}
	}

	/** Parses a string argument. 
	 */
	template<>
	inline void Arg<std::string>::parse(char const* value) {
		value_ = std::string(value);
		BaseArg::parse(value);
	}

	template<>
	inline void Arg<std::vector<std::string>>::parse(char const* value) {
		value_.push_back(value);
		specified_ = true;
	}




} // namespace helpers