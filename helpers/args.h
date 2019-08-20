#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <limits>
#include <stdexcept>
#include <iostream>

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

		friend std::ostream& operator << (std::ostream& s, BaseArg const& arg) {
			arg.print(s);
			return s;
		}

		friend class Arguments;

		/** Creates an argument. 

		    An argument has the following properties:

			aliases - prefixes which enable setting the value of the argument on the commandline
			position - either NOT_POSITIONAL if the argument does not have fixed position, or the position at which the argument is to be found in the command line.
			required - if true, the argument must be provided, if false it may be skipped. All positional arguments but the last one must be required.
			last - if the argument is last, any other arguments after it are not parsed as separate arguments, but as additional values to the argument.
			description - help

			TODO make this nicer
		 */
		BaseArg(std::initializer_list<char const*> aliases, int position, bool required, bool last, std::string const& description);

		/** Called by the arguments managed with the value intended for the argument. 

		    This method must be implemented by BaseArg descendants and the appropriate value must be parsed and the value updated. An exception should be thrown in case of any errors. 

			The default implementation stub simply throws if the value has already been specified before.
		 */
		virtual void parse(char const* value) {
			MARK_AS_UNUSED(value);
			if (specified_)
				THROW(ArgumentError()) << "Argument " << name_ << " already specified.";
		}

		virtual void print(std::ostream& s) const {
			s << name_;
			for (auto i : aliases_)
				if (i != name_)
					s << ", " << i;
			if (required_)
				s << " [required]";
			s << std::endl;
			s << "    " << description_;
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
		Arg(std::initializer_list<char const*> aliases, T const& value, bool required = false, std::string const& description = "", bool isLast = false, int position = BaseArg::NOT_POSITIONAL) :
			BaseArg(aliases, position, required, isLast, description),
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

		T & operator * () {
			return value_;
		}

		T * operator ->() {
			return &value_;
		}

	protected:

		/** This must be overriden by the specifications. 
		 */
		void parse(char const* value) override;

		void print(std::ostream& s) const override {
			BaseArg::print(s);
			if (!required_)
			    s << std::endl << "    Value: " << value_;
		}

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

		/** Determines whether unknown arguments are supported or not.
		 */
		static void AllowUnknownArguments(bool value = true) {
			ArgumentsList().allowUnknownArgs = value;
		}

		/** Returns a map of unknown arguments and their values in case unknown arguments are allowed by the implementation. 
		 */
		static std::unordered_map<std::string, std::string> const& UnknownArguments() {
			return ArgumentsList().unknownArgs;
		}

		/** Returns the command line used to run the application.

		    This is the first argument given to the app. 
		 */
		static std::string const& CommandLine() {
			return ArgumentsList().commandLine;
		}

		static std::string Print(int argc, char* argv[]) {
			std::stringstream result;
			for (int i = 0; i < argc; ++i) {
				if (i != 0)
					result << " ";
				result << argv[i++];
			}
			return result.str();
		}
		

		/** Parses the given commandline. 
		 */
		static void Parse(int argc, char* argv[]) {
			int i = 0;
			try {
				Impl& x = ArgumentsList();
				if (argc == 2) {
					if (strncmp(argv[1], "--version", 9) == 0 && !x.version.empty()) {
						std::cout << x.version << std::endl;
						exit(EXIT_SUCCESS);
					}
					if (strncmp(argv[1], "--help", 6) == 0) {
						Help(std::cout);
						exit(EXIT_SUCCESS);
					}
				}
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
				Help(std::cout);
				std::cerr << "ERROR: " << e << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}

		/** Prints the usage help. 
		 */
		static void Help(std::ostream& s) {
			Impl& x = ArgumentsList();
			if (!x.description.empty())
				s << x.description << std::endl;
			if (!x.usage.empty())
				s << "Usage: " << std::endl << x.usage;
			s << "" << std::endl << "Argument Details:" << std::endl << std::endl;
			for (auto i : x.byName)
				s << (*i.second) << std::endl;
			s << std::endl;
		}

		static void SetDescription(std::string const& description) {
			ArgumentsList().description = description;
		}

		static void SetUsage(std::string const& usage) {
			ArgumentsList().usage = usage;
		}

		static void SetVersion(std::string const& version) {
			ArgumentsList().version = version;
		}

	private:
		friend class BaseArg;

		class Impl {
		public:
			std::string version;
			std::string description;
			std::string usage;
			std::string commandLine;
			std::unordered_map<std::string, BaseArg*> byAlias;
			std::unordered_map<std::string, BaseArg*> byName;
			std::vector<BaseArg*> byPosition;
			std::unordered_map<std::string, std::string> unknownArgs;
			bool allowUnknownArgs = false;
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
					if (arg == args.byAlias.end()) {
						if (! args.allowUnknownArgs)
							THROW(ArgumentError()) << "Unrecognized argument " << argName;
						// if unknown arguments are allowed, we have one
						args.unknownArgs.insert(std::make_pair(argName, (argValue == nullptr) ? std::string() : std::string(argValue)));
						// and move to next argument
						++i;
						continue;
					}
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

	inline BaseArg::BaseArg(std::initializer_list<char const*> aliases, int position, bool required, bool last, std::string const& description) :
	    name_(*aliases.begin()),
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
			ASSERT(required || last) << "All but last positional arguments must be required for argument " << name_;
			if (static_cast<size_t>(position) >= x.byPosition.size())
				x.byPosition.resize(position + 1, nullptr);
			ASSERT(x.byPosition[position] == nullptr) << "Expected position " << position << " already taken by argument " << x.byPosition[position]->name() <<  " for argument " << name_;
			ASSERT(position == 0 || x.byPosition[position - 1]->last_ == false) << "Only last positional argument can be designated as last argument (argument " << name_ << ")";
			x.byPosition[position] = this;
		}
	}

	/** String argument. 
	 */
	template<>
	inline void Arg<std::string>::parse(char const* value) {
		BaseArg::parse(value);
		value_ = std::string(value);
	}

	/** Boolean argument. 

	    Together with disabled expected value setter. 
	 */
	template<>
	inline void Arg<bool>::parse(char const* value) {
		// when no value is provided, the default value is negated. 
		if (value == nullptr) {
			value_ = ! value_;
		} else {
			THROW(ArgumentError()) << "Value cannot be specified for argument " << name_;
		}
	}

	template<>
	inline bool Arg<bool>::expectsValue() const {
		return required_;
	}

	/** Unsigned argument. 
	 */
	template<>
	inline void Arg<unsigned>::parse(char const* value) {
		try {
			BaseArg::parse(value);
			size_t pos;
			std::string val(value);
		    unsigned long x = std::stoul(val, & pos);
			if (pos < val.size())
				throw std::invalid_argument("");
			if (x > std::numeric_limits<unsigned>::max())
				throw std::out_of_range("");
			value_ = static_cast<unsigned>(x);
		} catch (std::invalid_argument const &) {
			THROW(ArgumentError()) << "Invalid value given for argument " << name_ << ", expected positive number but " << value << " found.";
		} catch (std::out_of_range const &) {
			THROW(ArgumentError()) << "Value for argument " << name_ << " too large";
		}
	}


	/** Multiple string argument.

	    Together with a print method. 
	 */
	template<>
	inline void Arg<std::vector<std::string>>::parse(char const* value) {
		// if it is the first value, delete any default value that might have been present
		if (specified_ == false)
		    value_.clear();
		value_.push_back(value);
		specified_ = true;
	}

	template<>
	inline void Arg<std::vector<std::string>>::print(std::ostream& s) const {
		BaseArg::print(s);
		if (!required_) {
			s << std::endl << "    Value:";
			for (auto i : value_)
				s << " " << i;
		}
	}




} // namespace helpers