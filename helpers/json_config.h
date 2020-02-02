#pragma once

#include <stack>
#include <string>
#include <functional>

#include "json.h"

namespace helpers {

	/** Exceptiomn thrown when invalid arguments are passed to the application. 
	 */
	class ArgumentError : public Exception {
	};

	/** JSON Based Configuration. 
	 
	    - default value specified as JSON text, always
		- required means it must be in the configuration file, 
		- if not found, its default value can be calculated 
	 
	 */

    class JSONConfig {
	public:
	    class Group;
		
		template<typename T>
		class Array;

		template<typename T>
		class Option;

		class Root;

	protected:

	    friend class JSONArguments;

		class ElementBase;

	    friend class ElementBase;

		friend class Group;

		template<typename T>
		friend class Option;

		template<typename T>
		friend class Array;

		JSONConfig():
		    config_{JSON::Kind::Object} {
		}

	    /** The actual configuration stored as a JSON. 
		 */
	    JSON config_;

		// TODO this should take the ElementBase as an argument so that error messages can be informative
		template<typename T>
		static T ParseValue(JSON const & json);

		static Group * GetActiveGroup() {
			auto & groups = ActiveGroups();
			ASSERT(!groups.empty()) << "No active group present";
			return groups.back();
		}

		static void PushActiveGroup(Group * group) {
			ActiveGroups().push_back(group);
		}

		static void PopActiveGroup(Group * group) {
			ASSERT(GetActiveGroup() == group);
			ActiveGroups().pop_back();
		}

	private:

		static std::vector<Group*> & ActiveGroups() {
			static std::vector<Group*> singleton;
			return singleton;
		}
	};

	template<>
	inline bool JSONConfig::ParseValue(JSON const & json) {
		if (json.kind() != JSON::Kind::Boolean)
		    THROW(JSONError()) << "Expected bool, but " << json << " found";
		return json.toBool();
	}

	template<>
	inline std::string JSONConfig::ParseValue(JSON const & json) {
		if (json.kind() != JSON::Kind::String)
		    THROW(JSONError()) << "Expected string, but " << json << " found";
		return json.toString();
	}

	template<>
	inline int JSONConfig::ParseValue(JSON const & json) {
		if (json.kind() != JSON::Kind::Integer)
		    THROW(JSONError()) << "Expected integer, but " << json << " found";
		return json.toInt();
	}

	template<>
	inline unsigned JSONConfig::ParseValue(JSON const & json) {
		if (json.kind() != JSON::Kind::Integer)
		    THROW(JSONError()) << "Expected unsigned, but " << json << " found";
		return json.toUnsigned();
	}

	template<>
	inline double JSONConfig::ParseValue(JSON const & json) {
		if (json.kind() != JSON::Kind::Double)
		    THROW(JSONError()) << "Expected double, but " << json << " found";
		return json.toDouble();
	}

	class JSONConfig::ElementBase {
	public:
	    virtual ~ElementBase() { }

		std::string const & description() const {
			return description_;
		}

		bool specified() const {
			return specified_;
		}

		/** Sets the value of the element from a string containing the JSON representation. 
		 */
		void set(std::string const & value) {
			specify(JSON::Parse(value), [](std::exception const & e){
				THROW(ArgumentError()) << e.what();
			});
		}

	protected:

		friend class JSONArguments;

	    friend class Group;

		ElementBase(std::string const & description):
		    json_{nullptr},
			description_{description},
			specified_{false} {
		}

		/** Specifies the value of the element from the given JSON. 
		 
		    Calls the provided error handler in case of unknown options. 
		 */
		virtual void specify(JSON const & from, std::function<void(std::exception const &)> errorHandler) = 0;

		/** Fixes missing default values. 
		 */
		virtual bool fixMissingDefaultValues() = 0;

		/** Returns new JSON object that contains the current settings. 
		 
		    If onlySpecified is true, then only the values which were explicitly specified (either by specify, or by fixMissingDefaultValues functions) will be stored. 
		 */
		virtual JSON save(bool onlySpecified = true) = 0;

	    JSON * json_;

		std::string description_;

		// true if the element was specified by the user
		bool specified_;
	}; 

	class JSONConfig::Group : public JSONConfig::ElementBase {
	public:

		Group(std::string const & name, std::string const & description):
		    ElementBase(description) {
			Group * parent = GetActiveGroup();
			PushActiveGroup(this);
			parent->addElement(*this, name, "{}");
		}

	protected:

		template<typename T>
	    friend class Option;

		// Constructor for root object where no parents are made
        Group(JSON * json):
		    ElementBase("") {
			PushActiveGroup(this);
			json_ = json;
		}

		void specify(JSON const & from, std::function<void(std::exception const &)> errorHandler) override {
			if (from.kind() != JSON::Kind::Object)
			    errorHandler(CREATE_EXCEPTION(JSONError()) << "Object expected, but " << from << " found");
			for (auto i = from.begin(), e = from.end(); i != e; ++i) {
				auto own = elements_.find(i.name());
				if (own == elements_.end())
					errorHandler(CREATE_EXCEPTION(JSONError()) << "Unknown option " << i.name());
				else
					own->second->specify(*i, errorHandler);
			}
			specified_ = true;
		}

		bool fixMissingDefaultValues() override {
			bool result = false;
			for (auto i : elements_) 
				result = i.second->fixMissingDefaultValues() || result;
			// mark the group as specified now and if it was not specified before, set the comments for readability too
			if (result && ! specified_) {
				json_->setComment(description_);
			    specified_ = true;
			}
			return result;
		}

		JSON save(bool onlySpecified = true) override {
			ASSERT(specified_ || ! onlySpecified);
			ASSERT(json_ != nullptr);
			JSON result{JSON::Kind::Object};
			for (auto i : elements_) {
				if (i.second->specified_ || ! onlySpecified)
				    result.add(i.first, i.second->save(onlySpecified));
			}
			return result;
		}


	    /** Adds the given element to the group.
		 */
	    void addElement(ElementBase & element, std::string const & name, std::string const & defaultValue) {
			ASSERT(elements_.find(name) == elements_.end()) << " Configuration element " << name << " already exists";
			elements_.insert(std::make_pair(name, &element));
			json_->add(name, JSON::Parse(defaultValue));
			element.json_ = & (*json_)[name];
		}

		void initializationDone() {
			PopActiveGroup(this);
		}

	    std::unordered_map<std::string, ElementBase *> elements_;
	};



	template<typename T>
	class JSONConfig::Array : public JSONConfig::ElementBase {
		// TODO TODO TODO 

	};

	template<typename T>
	class JSONConfig::Option : public JSONConfig::ElementBase {
	public:
		Option(std::string const & name, std::string const & description, std::string const & defaultValue):
		    ElementBase(description) {
			Group * parent = GetActiveGroup();
			ASSERT(parent != nullptr);
			parent->addElement(*this, name, defaultValue);
		}

		Option(std::string const & name, std::string const & description, std::function<std::string(void)> defaultValue):
		    ElementBase(description),
			defaultValue_{defaultValue} {
			Group * parent = GetActiveGroup();
			ASSERT(parent != nullptr);
			parent->addElement(*this, name, "null");
		}

		/** Returns the configuration value parsed from the stored JSON. 
		 */
		T operator () () const {
			return JSONConfig::ParseValue<T>(*json_);
		}

	protected:

		void specify(JSON const & from, std::function<void(std::exception const &)> errorHandler) override {
			MARK_AS_UNUSED(errorHandler);
			(*json_) = from;
			specified_ = true;
		}

		bool fixMissingDefaultValues() override {
			if (defaultValue_ && ! specified_) {
				// update the json with the calculated default value
				(*json_) = JSON::Parse(defaultValue_());
				// and set the description as a comment for better readability
				json_->setComment(description_);
				specified_ = true;
				return true;
			} else {
				return false;
			}
		}

		JSON save(bool onlySpecified = true) override {
			ASSERT(json_ != nullptr);
			// if the value was specified, just return the JSON 
			if (specified_) {
				return *json_;
			// otherwise create a copy and make sure that the comments are copied for better readability
			} else {
				ASSERT(! onlySpecified);
				JSON result{*json_};
				result.setComment(description_);
				return result;
			}
		}

	    std::function<std::string(void)> defaultValue_;

	}; // helpers::JSONConfig::Option

	class JSONConfig::Root : public JSONConfig, public JSONConfig::Group {
	public:

		std::string toString() {
			return STR(save(true));
		}

	protected:

	    Root():
		    JSONConfig{},
		    JSONConfig::Group{&config_} {
			// TODO check that there is only one active group and that is us
		}

	}; // helpers::JSONConfig::Root

	#define CONFIG_OPTION(NAME, DESCRIPTION, DEFAULT_VALUE, ...) \
	    Option<__VA_ARGS__> NAME{#NAME, DESCRIPTION, DEFAULT_VALUE}

	#define CONFIG_GROUP(NAME, DESCRIPTION, ...) \
	    class NAME ## Group : public helpers::JSONConfig::Group { \
		public: \
		    __VA_ARGS__ \
			NAME ## Group(): helpers::JSONConfig::Group{#NAME, DESCRIPTION} { \
			    initializationDone(); \
			} \
		}; \
		NAME ## Group NAME

	/** Mapping from commandline arguments to JSON configuration. 
	 
	    Analyzes the command line arguments and calculates their values for the existing configuration. This is done by keeping a mapping from command-line aliases to JSON configuration elements as templated setters. Both positional and keyword arguments are supported. 

		Unlike configuration, which is a global object, the arguments are expected to be created when needed and destroyed when the commandline is parsed as all their values are backed by respective configuration options anyways.  
	 */
    class JSONArguments {
	public:

	    JSONArguments():
		    defaultArgument_{nullptr} {
		}

	    ~JSONArguments() {
			for (auto i : arguments_) 
			    delete i.second;
		}

	    void parse(int argc, char * argv[]) {
			int i = 1;
			parsePositionalArguments(i, argc, argv);
			parseKeywordArguments(i, argc, argv);
			ASSERT(i == argc);
			for (auto j : arguments_)
			    j.second->finalizeAndVerify();
		}

		void setDefaultArgument(std::string const & name) {
			auto i = arguments_.find(name);
			ASSERT(i != arguments_.end()) << "Argument " << name << " does not exist";
			defaultArgument_ = i->second;
		}

		template<typename T>
		size_t addPositionalArgument(std::string const & name, JSONConfig::Option<T> & option) {
			ArgumentBase * arg = new Argument<T>{name, true, true, option};
			addArgument(arg);
			positionalArguments_.push_back(arg);
			return positionalArguments_.size() - 1;
		}

		template<typename T>
	    void addArgument(std::string const & name, std::initializer_list<char const *> aliases, JSONConfig::Option<T> & option, bool expectsValue = true, bool required = false) {
			ArgumentBase * arg = new Argument<T>{name, required, expectsValue, option};
			addArgument(arg);
			addAliases(arg, aliases);
		}

		template<typename T>
	    void addArrayArgument(std::string const & name, std::initializer_list<char const *> aliases, JSONConfig::Option<T> & option, bool last = false, bool required = false) {
			ArgumentBase * arg = new ArrayArgument<T>{name, required, last, option};
			addArgument(arg);
			addAliases(arg, aliases);
		}

	protected:

		/** Takes given value as defined in the command line and converts it to appropriate JSON string storing information about the given type. 
		 */
		template<typename T>
		static std::string ConvertToJSON(std::string const & value);

		void parsePositionalArguments(int & i, int argc, char * argv[]) {
			for (ArgumentBase * arg : positionalArguments_) {
				if (i == argc)
				    THROW(ArgumentError()) << "Argument " << arg->name() << " not provided";
				arg->parse(argv[i]);
				++i;
				// if the argument is last, parse all remaining arguments as its values
				// TODO this will lead to weird errors if other than last positional argument is selected as last
				if (arg->isLastArgument()) {
					while (i < argc) 
					    arg->parse(argv[i++]);
				}
			}
		}

		void parseKeywordArguments(int & i, int argc, char * argv[]) {
			while (i < argc) {
				char const * argValue = nullptr;
				std::string argName{argv[i]};
				auto arg = keywordArguments_.find(argName);
				// if the entire argument is not recognized argument alias, split it by the '='
				if (arg == keywordArguments_.end()) {
					size_t x = argName.find('=');
					if (x == std::string::npos) {
						if (defaultArgument_ != nullptr) {
						    defaultArgument_->parse(argName);
							++i;
							continue;
						} else {
					        THROW(ArgumentError()) << "Unknown argument name " << argName;
						}
					}
					argName = argName.substr(0, x);
					argValue = argv[i] + x + 1;
					arg = keywordArguments_.find(argName);
					if (arg == keywordArguments_.end()) {
						if (defaultArgument_ != nullptr) {
						    defaultArgument_->parse(argv[i]);
							++i;
							continue;
						} else {
					        THROW(ArgumentError()) << "Unknown argument name " << argName;
						}
					}
				}
				if (argValue == nullptr && arg->second->expectsValue()) {
					if (++i == argc)
				        THROW(ArgumentError()) << "Argument " << arg->second->name() << " value not provided";
					argValue = argv[i];
				}
				// parse the value
				arg->second->parse(argValue);
				++i;
				if (arg->second->isLastArgument()) {
					while (i < argc) 
					    arg->second->parse(argv[i++]);
				}
			}
		}


	private:

		/** Basic argument properties. 
		 */
		class ArgumentBase {
		public:
		    std::string const & name() const {
				return name_;
			}

		protected:

			friend class JSONArguments;

			ArgumentBase(std::string const & name, bool required, JSONConfig::ElementBase & element):
			    name_{name},
				description_{element.description()},
				required_{required},
				specified_{false},
				element_{element} {
			}

			virtual ~ArgumentBase() { }

			virtual bool isLastArgument() const {
				return false;
			}

			virtual bool expectsValue() const {
				return true;
			}

			virtual void parse(std::string const & value) = 0;

			virtual void finalizeAndVerify() = 0;

			// name of the argument (the default alias)
			std::string name_;
			// description of the argument 
			std::string description_;
			// determines whether the argument must be specified, or not
			bool required_;
			// determines whether the argument was specified on the commandline
			bool specified_;
			// the corresponding JSON configuration option
			JSONConfig::ElementBase & element_;
		}; // JSONArguments::ArgumentBase

		/** Single value argument that can only be specified once. 
		 
			Multiple specification will result in an error.
		*/
		template<typename T>
		class Argument : public ArgumentBase {
		public:
		    Argument(std::string const & name, bool required, bool expectsValue, JSONConfig::ElementBase & element):
			    ArgumentBase{name, required, element},
				expectsValue_{expectsValue} {
			}

		protected:

			bool expectsValue() const override {
				return expectsValue_;
			}

			void parse(std::string const & value) override {
				if (specified_)
				    THROW(ArgumentError()) << "Argument " << name() << " already specified";
				element_.specify(JSON::Parse(ConvertToJSON<T>(value)), [](std::exception const & e){
					THROW(ArgumentError()) << e.what();
				});
				specified_ = true;
			}

			/** Finalizing single value arguments is easy, only checks that if the argument is required it was specified. 
			 */
			void finalizeAndVerify() override {
				if (required_ && ! specified_)
				    THROW(ArgumentError()) << "Argument " << name() << " required";
			}

			bool expectsValue_;

		}; // JSONArguments::Argument

		/** An array argument which can contain multiple values. 
		 
			By default these are stored as a JSON array of particular data types, but overriding the template behavior for specific types can alter this.
		*/
		template<typename T>
		class ArrayArgument : public ArgumentBase {
		public:

		    ArrayArgument(std::string const & name, bool required, bool last, JSONConfig::ElementBase & element):
			    ArgumentBase{name, required, element},
				last_{last} {
			}

		protected:

		    bool isLastArgument() const override {
				return last_;
			}

			void parse(std::string const & value) override {
				if (!value_.empty())
				    value_ += ", ";
				value_ += ConvertToJSON<T>(value);
				specified_ = true;
			}

			/** Finalizing array argument makes sure that if required, the argument was specified and also sets the collected value as the JSON for the configuration option. 
			 */
			void finalizeAndVerify() override {
				if (required_ && ! specified_)
				    THROW(ArgumentError()) << "Argument " << name() << " required";
				// sets the value as JSON for the attached option
				if (specified_) {
					element_.specify(JSON::Parse(STR("[" << value_ << "]")), [](std::exception const & e){
						THROW(ArgumentError()) << e.what();
					});
				}
			}

			bool last_;
			// temporary value as the multiple values are parsed (if any)
			std::string value_;
		}; // JSONArguments::ArrayArgument

	    void addArgument(ArgumentBase * arg) {
			ASSERT(arguments_.find(arg->name()) == arguments_.end()) << "Argument " << arg->name() << " already defined";
			arguments_.insert(std::make_pair(arg->name(), arg));
		}

		void addAliases(ArgumentBase * arg, std::initializer_list<char const *> aliases) {
			for (char const * alias : aliases) {
				std::string a{alias};
				ASSERT(keywordArguments_.find(a) == keywordArguments_.end()) << "Argument alias " << a << " already defined";
				keywordArguments_.insert(std::make_pair(a, arg));
			}
		}

		// all registered arguments
		std::unordered_map<std::string, ArgumentBase *> arguments_;
		// positional arguments
	    std::vector<ArgumentBase *> positionalArguments_;
		// keyword arguments
		std::unordered_map<std::string, ArgumentBase *> keywordArguments_;
		// default argument
		ArgumentBase * defaultArgument_;

	}; // helpers::JSONArguments


	template<>
	inline std::string JSONArguments::ConvertToJSON<std::string>(std::string const & value) {
        return STR("\"" << value << "\"");
	}

	template<>
	inline std::string JSONArguments::ConvertToJSON<int>(std::string const & value) {
		return value;
	}

	template<>
	inline std::string JSONArguments::ConvertToJSON<unsigned>(std::string const & value) {
		return value;
	}

	template<>
	inline std::string JSONArguments::ConvertToJSON<double>(std::string const & value) {
		return value;
	}

} // namespace helpers