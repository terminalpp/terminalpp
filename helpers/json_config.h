#pragma once

#include <stack>
#include <string>
#include <functional>
#include <variant>

#include "json.h"

HELPERS_NAMESPACE_BEGIN

	#define CONFIG_PROPERTY(NAME, DESCRIPTION, DEFAULT_VALUE, ...) \
	    Property<__VA_ARGS__> NAME{this, #NAME, DESCRIPTION, DEFAULT_VALUE}

	#define CONFIG_OBJECT(NAME, DESCRIPTION, ...) \
	    class NAME ## Object : public Object { \
		public: \
		    __VA_ARGS__ \
			NAME ## Object(JSONConfig * parent): Object{parent, #NAME, DESCRIPTION} { \
			} \
		}; \
		NAME ## Object NAME{this}

    #define CONFIG_ARRAY(NAME, DESCRIPTION, DEFAULT_VALUE, ...) \
        class NAME ## _entry : public Object { \
        public: \
		    __VA_ARGS__ \
			NAME ## _entry(JSONConfig * parent): Object{parent, "", ""} { \
            } \
        }; \
        Array<NAME ## _entry> NAME{this, #NAME, DESCRIPTION, DEFAULT_VALUE}


    namespace x {

        /** - each config knows its parent - don't deal with calculated-able default values
         
            - can return default JSON

            - initially has no JSON, when updated its JSON is set

            - empty value == default value, by which the configuration is initialized

         */

        class JSONConfig {
        public:

            class Root;

            class Object;

            template<typename T>
            class Array;

            template<typename T>
            class Property;

            virtual ~JSONConfig() {
            }

            std::string name() const {
                if (parent_ == nullptr)
                    return nullptr;
                else
                    return parent_->childName(this);
            }

        protected:

            JSONConfig(JSONConfig * parent, std::string const & name, std::string const & description, JSON const & defaultValue):
                parent_{parent},
                json_{nullptr},
                description_{description},
                defaultValue_{defaultValue},
                updated_{false} {
                if (parent_ != nullptr)
                    parent_->addChildProperty(name, this);
                // if parent is null, then we are root and should create the root object 
                else 
                    json_ = new JSON{JSON::Kind::Object};
            }

            JSONConfig(JSONConfig * parent, std::string const & name, std::string const & description, std::function<JSON()> defaultValue):
                parent_{parent},
                json_{nullptr},
                description_{description},
                defaultValue_{defaultValue},
                updated_{false} {
                if (parent_ != nullptr)
                    parent_->addChildProperty(name, this);
                // if parent is null, then we are root and should create the root object 
                else 
                    json_ = new JSON{JSON::Kind::Object};
            }

            JSON defaultValue() const {
                if (std::holds_alternative<JSON>(defaultValue_))
                    return std::get<JSON>(defaultValue_);
                else
                    return std::get<std::function<JSON()>>(defaultValue_)();
            }

            void update(JSON const & value) {
                update(value, [](JSONError && e){
                    throw e;
                });
            }

            template<typename T>
            static T FromJSON(JSON const & json);

            virtual void update(JSON const & value, std::function<void(JSONError &&)> errorHandler) = 0;

            /** Calls update on given config. 
             
                Required due to friend delcarations only valid on this object, so that subclasses have no way of calling update on other items than themselves.
             */
            void update(JSONConfig * config, JSON const & value, std::function<void(JSONError &&)> errorHandler) {
                config->update(value, errorHandler);
            }

            virtual void addChildProperty(std::string const & name, JSONConfig * child) = 0;

            /** Returns the fully qualified name of the given child of the configuration element. 
             
                The child's element name is returned as opposed to own name because the parent determines the location of the child in it (i.e. a named property, or array index). 
             */            
            virtual std::string childName(JSONConfig const * child) const = 0;

            /** Parent configuration object. */
            JSONConfig * parent_;
            /* The JSON object holding the configuration. */
            JSON * json_;

            std::string description_;

            std::variant<JSON, std::function<JSON()>> defaultValue_;

            bool updated_;

        }; 

        class JSONConfig::Object : public JSONConfig {
        public:
            Object(JSONConfig * parent, std::string const & name, std::string const & description):
                JSONConfig{parent, name, description, JSON::Object()} {
                //ASSERT(parent != nullptr) << "Use JSONConfig::Root for parent-less configuration objects";
            }

        protected:

            void update(JSON const & value, std::function<void(JSONError &&)> errorHandler) override {
                ASSERT(json_ != nullptr);
                if (value.kind() != JSON::Kind::Object) {
                    errorHandler(CREATE_EXCEPTION(JSONError())  << "Initializing " << name() << " with " << value << ", but object expected");
                    return;
                }
                updated_ = true;
                json_->setComment(value.comment());
                // update the values
                for (auto i = value.begin(), e = value.end(); i != e; ++i) {
                    auto x = properties_.find(i.name());
                    if (x == properties_.end()) 
                        errorHandler(CREATE_EXCEPTION(JSONError()) << "Unknown property " << i.name() << " in " << name());
                    else
                        x->second->update(*i);
                }
                // if there are any properties that were not updated yet, update them with their default values and clear the updated_ flag
                for (auto i : properties_) {
                    JSONConfig * child = i.second;
                    if (! child->updated_) {
                        child->update(child->defaultValue()); // this is the default value, therefore errors are more serious and should be reported immediately
                        // if the default value has not been calculated, but provided as a literal, mark the field as *not* updated (no need to store)
                        if (std::holds_alternative<JSON>(child->defaultValue_))
                            child->updated_ = false; // clear the update flag
                    }
                }
            }

            void addChildProperty(std::string const & name, JSONConfig * child) override {
                if (properties_.insert(std::make_pair(name, child)).second == false)
                    THROW(JSONError()) << "Element " << name << " already exists in " << this->name();
                // add the object placeholder to parent - this *will* be changed to parsed value, or to the default value before first parsed, but the object will allow nesting
                child->json_ = & json_->add(name, JSON::Object());
            }

            std::string childName(JSONConfig const * child) const override {
                std::string result = (parent_ == nullptr) ? "" : (parent_->childName(this) + '.');
                for (auto i : properties_)
                    if (i.second == child)
                        return result + i.first;
                UNREACHABLE;
            }

            std::unordered_map<std::string, JSONConfig*> properties_;

        }; // JSONConfig::Object

        template<typename T>
        class JSONConfig::Array : public JSONConfig {
        public:
            Array(JSONConfig * parent, std::string const & name, std::string const & description):
                JSONConfig{parent, name, description, JSON::Array()} {
            }

            Array(JSONConfig * parent, std::string const & name, std::string const & description, JSON const & defaultValue):
                JSONConfig{parent, name, description, defaultValue} {
            }

            Array(JSONConfig * parent, std::string const & name, std::string const & description, std::function<JSON()> defaultValue):
                JSONConfig{parent, name, description, defaultValue} {
            }

            T const & operator [] (size_t index) const {
                ASSERT(index < elements_.size());
                return * dynamic_cast<T*>(elements_[index]);
            }

        protected:

            void update(JSON const & value, std::function<void(JSONError &&)> errorHandler) override {
                ASSERT(json_ != nullptr);
                *json_ = JSON::Array();
                if (value.kind() != JSON::Kind::Array) {
                    errorHandler(CREATE_EXCEPTION(JSONError())  << "Initializing " << name() << " with " << value << ", but array expected");
                    return;
                }
                updated_ = true;
                json_->setComment(value.comment());
                // delete previous value
                for (auto i : elements_)
                    delete i;
                elements_.clear();
                // update the values
                for (auto i = value.begin(), e = value.end(); i != e; ++i) {
                    T * element = new T{this};
                    JSONConfig::update(element, *i, errorHandler);
                }
            }

            void addChildProperty(std::string const & name, JSONConfig * child) override {
                ASSERT(name == "");
                elements_.push_back(child);
                child->json_ = & json_->add(JSON::Object());
            }

            std::string childName(JSONConfig const * child) const override {
                std::string result = (parent_ == nullptr) ? "" : (parent_->childName(this) + '[');
                for (size_t i = 0, e = elements_.size(); i != e; ++i)
                    if (elements_[i] == child)
                        return STR(result << i << "]");
                UNREACHABLE;
            }

            std::vector<JSONConfig*> elements_;

        }; // JSONConfig::Array

        template<typename T>
        class JSONConfig::Property : public JSONConfig {
        public:

            Property(JSONConfig * parent, std::string name, std::string description, JSON const & defaultValue):
                JSONConfig{parent, name, description, defaultValue} {
            }

            Property(JSONConfig * parent, std::string name, std::string description, std::function<JSON()> defaultValue):
                JSONConfig{parent, name, description, defaultValue} {
            }
              
            /** Typecasts the configuration property to the property value type. 
             */
            T const & operator () () const {
                return value_;
            }

        protected:

            void update(JSON const & value, std::function<void(JSONError &&)> errorHandler) override {
                ASSERT(json_ != nullptr);
                try {
                    value_ = JSONConfig::FromJSON<T>(value);
                    json_->setComment(value.comment());
                    updated_ = true;
                } catch (std::exception const & e) {
                    errorHandler(CREATE_EXCEPTION(JSONError()) << "Error when parsing JSON value for " << name() << ": " << e.what());
                }
            }

            void addChildProperty(std::string const & name, JSONConfig * child) override {
                MARK_AS_UNUSED(name);
                MARK_AS_UNUSED(child);
                UNREACHABLE;
            }

            std::string childName(JSONConfig const * child) const override {
                MARK_AS_UNUSED(child);
                UNREACHABLE;
            }

            T value_;
        }; 


        /** The root element of the JSON backed configuration. 
         */
        class JSONConfig::Root : public JSONConfig::Object {
        public:

            Root():
                Object{nullptr, "", "Configuration"} {
            }

            Root(std::string description):
                Object{ nullptr, "", description } {
            }

            ~Root() {
                delete json_;
            }

        protected:

            using JSONConfig::update;
            using Object::update;

        }; // JSONConfig::Root

        template<>
        inline std::string JSONConfig::FromJSON<std::string>(JSON const & json) {
            if (json.kind() != JSON::Kind::String)
                THROW(JSONError()) << "Expected string, but " << json << " found";
            return json.toString();
        }

        template<>
        inline bool JSONConfig::FromJSON<bool>(JSON const & json) {
            if (json.kind() != JSON::Kind::Boolean)
                THROW(JSONError()) << "Expected bool, but " << json << " found";
            return json.toBool();
        }

        template<>
        inline int JSONConfig::FromJSON<int>(JSON const & json) {
            if (json.kind() != JSON::Kind::Integer)
                THROW(JSONError()) << "Expected integer, but " << json << " found";
            return json.toInt();
        }

        template<>
        inline unsigned JSONConfig::FromJSON<unsigned>(JSON const & json) {
            if (json.kind() != JSON::Kind::Integer)
                THROW(JSONError()) << "Expected unsigned, but " << json << " found";
            return json.toUnsigned();
        }

        template<>
        inline size_t JSONConfig::FromJSON<size_t>(JSON const & json) {
            if (json.kind() != JSON::Kind::Integer)
                THROW(JSONError()) << "Expected unsigned, but " << json << " found";
            return json.toUnsigned();
        }

        template<>
        inline double JSONConfig::FromJSON<double>(JSON const & json) {
            if (json.kind() != JSON::Kind::Double)
                THROW(JSONError()) << "Expected double, but " << json << " found";
            return json.toDouble();
        }

    } // X -----------------------------------------------------------------------------------

	/** Exceptiomn thrown when invalid arguments are passed to the application. 
	 */
	class ArgumentError : public Exception {
	};

	/** JSON Based Configuration.
	 */
    class JSONConfig {
	public:
	    class Group;
		
		template<typename T>
		class Array;

		template<typename T>
		class Option;

		class Root;

		virtual ~JSONConfig() {
		}

		std::string name() const {
			if (parent_ == nullptr || parent_->parent_ == nullptr)
			    return name_;
			else
			    return parent_->name() + "." + name_;
		}

		std::string const & description() const {
			return description_;
		}

		bool specified() const {
			return specified_;
		}

		/** Explicitly sets the value of the element from a string containing the JSON representation. 
		 */
		void set(std::string const & value) {
			specify(JSON::Parse(value), [](std::exception const & e){
				THROW(ArgumentError()) << e.what();
			});
		}

	protected:

	    friend class JSONArguments;

		friend class Group;

		template<typename T>
		friend class Option;

		template<typename T>
		friend class Array;

		JSONConfig(std::string const & name, std::string const & description):
		    json_{nullptr},
			name_{name},
			description_{description},
			specified_{false},
			parent_{nullptr} {
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

		/** Name of the element. 
		 */
		std::string name_;

		std::string description_;

		// true if the element was specified by the user
		bool specified_;

		/** Parent element if any, nullptr for root element. 
		 */
		JSONConfig * parent_;

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
            MARK_AS_UNUSED(group);
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

    template<>
    inline std::vector<std::string> JSONConfig::ParseValue(JSON const & json) {
        std::vector<std::string> result;
        if (json.kind() != JSON::Kind::Array)
		    THROW(JSONError()) << "Expected array, but " << json << " found";
        for (auto & item : json) {
            if (item.kind() != JSON::Kind::String)
    		    THROW(JSONError()) << "Strings expected in the array, but  " << item << " found";
            result.push_back(item.toString());
        }
        return result;
    }

	class JSONConfig::Group : public JSONConfig {
	public:

		Group(std::string const & name, std::string const & description):
		    JSONConfig(name, description) {
			Group * parent = GetActiveGroup();
			PushActiveGroup(this);
			parent->addElement(*this, name, "{}");
		}

	protected:

		template<typename T>
	    friend class Option;

		// Constructor for root object where no parents are made
        Group():
		    JSONConfig("", "") {
			PushActiveGroup(this);
		}

		void specify(JSON const & from, std::function<void(std::exception const &)> errorHandler) override {
			if (from.kind() != JSON::Kind::Object)
			    errorHandler(CREATE_EXCEPTION(JSONError()) << "Object expected, but " << from << " found in " << name());
			for (auto i = from.begin(), e = from.end(); i != e; ++i) {
				auto own = elements_.find(i.name());
				if (own == elements_.end())
					errorHandler(CREATE_EXCEPTION(JSONError()) << "Unknown option " << i.name() << " in " << name());
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
	    void addElement(JSONConfig & element, std::string const & name, std::string const & defaultValue) {
			ASSERT(elements_.find(name) == elements_.end()) << " Configuration element " << name << " already exists";
			elements_.insert(std::make_pair(name, &element));
			json_->add(name, JSON::Parse(defaultValue));
			element.json_ = & (*json_)[name];
			ASSERT(element.parent_ == nullptr) << "Element already has parent";
			element.parent_ = this;
		}

		void initializationDone() {
			PopActiveGroup(this);
		}

	    std::unordered_map<std::string, JSONConfig *> elements_;
	};



	template<typename T>
	class JSONConfig::Array : public JSONConfig {
		// TODO TODO TODO 

	};

	template<typename T>
	class JSONConfig::Option : public JSONConfig {
	public:
		Option(std::string const & name, std::string const & description, std::string const & defaultValue):
		    JSONConfig(name, description) {
			Group * parent = GetActiveGroup();
			ASSERT(parent != nullptr);
			parent->addElement(*this, name, defaultValue);
		}

		Option(std::string const & name, std::string const & description, std::function<std::string(void)> defaultValue):
		    JSONConfig(name, description),
			defaultValue_{defaultValue} {
			Group * parent = GetActiveGroup();
			ASSERT(parent != nullptr);
			parent->addElement(*this, name, "null");
		}

		/** Returns the configuration value parsed from the stored JSON. 
		 */
		T operator () () const {
			try {
			    return JSONConfig::ParseValue<T>(*json_);
			} catch (Exception & e) {
				e.setMessage(STR(" in confifuration value " << name()));
				throw e;
			}
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
                MARK_AS_UNUSED(onlySpecified);
				JSON result{*json_};
				result.setComment(description_);
				return result;
			}
		}

	    std::function<std::string(void)> defaultValue_;

	}; // JSONConfig::Option

	class JSONConfig::Root : public JSONConfig::Group {
	public:

		std::string toString() {
			return STR(save(true));
		}

	protected:

	    Root():
		    JSONConfig::Group{},
			config_{JSON::Kind::Object} {
			// set the JSON element for the group to the root JSON element
			json_ = &config_;
			// TODO check that there is only one active group and that is us
		}

		/** JSON file storing the configuration
		 */ 
		JSON config_;

	}; // JSONConfig::Root

	#define CONFIG_OPTION(NAME, DESCRIPTION, DEFAULT_VALUE, ...) \
	    Option<__VA_ARGS__> NAME{#NAME, DESCRIPTION, DEFAULT_VALUE}

	#define CONFIG_GROUP(NAME, DESCRIPTION, ...) \
	    class NAME ## Group : public HELPERS_NAMESPACE_DECL::JSONConfig::Group { \
		public: \
		    __VA_ARGS__ \
			NAME ## Group(): HELPERS_NAMESPACE_DECL::JSONConfig::Group{#NAME, DESCRIPTION} { \
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
				arg->second->parse(argValue == nullptr ? "" : argValue);
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

			ArgumentBase(std::string const & name, bool required, JSONConfig & element):
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
			JSONConfig & element_;
		}; // JSONArguments::ArgumentBase

		/** Single value argument that can only be specified once. 
		 
			Multiple specification will result in an error.
		*/
		template<typename T>
		class Argument : public ArgumentBase {
		public:
		    Argument(std::string const & name, bool required, bool expectsValue, JSONConfig & element):
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

		    ArrayArgument(std::string const & name, bool required, bool last, JSONConfig & element):
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

	}; // JSONArguments


	template<>
	inline std::string JSONArguments::ConvertToJSON<std::string>(std::string const & value) {
        return STR("\"" << value << "\"");
	}

    template<>
    inline std::string JSONArguments::ConvertToJSON<bool>(std::string const & value) {
        return value;
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

HELPERS_NAMESPACE_END