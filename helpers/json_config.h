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

    class ArgumentError : public Exception {
    };

    /** JSON Backed Configuration
     
        - can return default JSON

        - initially has no JSON, when updated its JSON is set

        - empty value == default value, by which the configuration is initialized

        */

    class JSONConfig {
    public:

        class Root;
        class CmdArgsRoot;

        class Object;

        template<typename T>
        class Array;

        template<typename T>
        class Property;

        virtual ~JSONConfig() {
            if (parent_ == nullptr)
                delete json_;
        }

        /** Returns the full name of the configuration option. 
         
            The name consists of the name of the property preceded by names of its parents separated by `.`.  
         */
        std::string name() const {
            if (parent_ == nullptr)
                return "";
            else
                return parent_->childName(this);
        }

        /** Returns the description of the property. 
         
            For default values, the description is also stored as a comment in the backing JSON object. 
         */
        std::string const & description() const {
            return description_;
        }

        /** Determines whether the value of the property has been updated or calculated. 
         
            A property's value is either the default JSON value, or it can be supplied by user (via the update() method call), or it can be the default calculated value. 

            If the value has been calculated, or provided by user, the updated() method returns true, false is returned otherwise. 
         */
        bool updated() const {
            return updated_;
        }

        /** Sets the value of the property from given JSON. 
         */
        void set(JSON const & value) {
            update(value);
        }

        /** Stores the value of the property (and subfields, if any) into a JSON. 
         
            By default only updated properties (i.e. computed defaults, or user specified values) are stored in the JSON, but if the `updatedOnly` argument is set to false, all fields will be saved. 
         */
        virtual JSON toJSON(bool updatedOnly = true) const = 0;

    protected:

        friend class CmdArgRoot;

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

        /** Converts the given JSON to a value of specified type. 
         
            Specializing the template for own types allows simple addition of properties of new user types. 
         */
        template<typename T>
        static T FromJSON(JSON const & json);

        /** Updates the value of the property with given JSON. 

            Returns whether any of the child properties (if any) were updated to user specified, or calculated default values.  
         */
        virtual bool update(JSON const & value, std::function<void(JSONError &&)> errorHandler = [](JSONError && e) { throw e; }) = 0;

        /** Returns the default value for the property. 
         
            This can either be a static JSON object, or dynamically computed JSON value, in which case the associated function to determine the value is called and its result returned. 

            In both cases the comment of the returned JSON is set to the description of the property.
         */
        JSON defaultValue() const {
            if (std::holds_alternative<JSON>(defaultValue_)) {
                JSON result = std::get<JSON>(defaultValue_);
                result.setComment(description_);
                return result;
            } else {
                JSON result = std::get<std::function<JSON()>>(defaultValue_)();
                result.setComment(description_);
                return result;
            }
        }

        /** Calls update on given config. 
         
            Required due to friend delcarations only valid on this object, so that subclasses have no way of calling update on other items than themselves.
            */
        bool updateOther(JSONConfig * config, JSON const & value, std::function<void(JSONError &&)> errorHandler = [](JSONError && e) { throw e; }) {
            return config->update(value, errorHandler);
        }

        virtual void addChildProperty(std::string const & name, JSONConfig * child) = 0;

        /** Returns the fully qualified name of the given child of the configuration element. 
         
            The child's element name is returned as opposed to own name because the parent determines the location of the child in it (i.e. a named property, or array index). 
            */            
        virtual std::string childName(JSONConfig const * child) const = 0;

        /** \name Command line arguments support. 
         */
        //@{

        /** Determines if the configuration explicitly requires value when used on commandline.
         */
        virtual bool cmdArgRequiresValue() const {
            return true;
        }

        /** Called when the value is updated from command line argument. 
         
            The index argument determines now many times the value has been set already so that array values can be implemented and errors can be raised for multiple values of non-array options. 

            The default implementation simply converts the string to JSON and calls the update method if this is first invocation, or throws an error if multiple invocations. 
            */
        virtual void cmdArgUpdate(char const * value, size_t index) {
            if (index != 0)
                THROW(JSONError()) << "Value for " << name() << " already provided";
            update(JSON::Parse(value), [](JSONError && e) { throw e; });
        }

        //@}

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

        JSON toJSON(bool updatedOnly = true) const override {
            ASSERT(json_ != nullptr);
            JSON result = JSON::Object();
            result.setComment(json_->comment());
            for (auto i : properties_) {
                if (updatedOnly && ! i.second->updated_)
                    continue;
                result.add(i.first, i.second->toJSON(updatedOnly));
            }
            return result;
        }

    protected:

        bool update(JSON const & value, std::function<void(JSONError &&)> errorHandler = [](JSONError && e) { throw e; }) override {
            ASSERT(json_ != nullptr);
            if (value.kind() != JSON::Kind::Object) {
                errorHandler(CREATE_EXCEPTION(JSONError())  << "Initializing " << name() << " with " << value << ", but object expected");
                return false;
            }
            updated_ = true;
            bool result = false;
            json_->setComment(value.comment());
            // update the values
            for (auto i = value.begin(), e = value.end(); i != e; ++i) {
                auto x = properties_.find(i.name());
                if (x == properties_.end()) 
                    errorHandler(CREATE_EXCEPTION(JSONError()) << "Unknown property " << i.name() << " in " << name());
                else
                    result = x->second->update(*i, errorHandler) || result;
            }
            // if there are any properties that were not updated yet, update them with their default values and clear the updated_ flag
            for (auto i : properties_) {
                JSONConfig * child = i.second;
                if (! child->updated_) {
                    if (child->update(child->defaultValue()) || std::holds_alternative<std::function<JSON()>>(child->defaultValue_)) {
                        result = true;
                        // if the default value is to be stored, we must propagate the updated flag to parent objects since they do not have currently to be flagged as updated
                        JSONConfig * x = this;
                        while (x != nullptr && x->updated_ == false) {
                            x->updated_ = true;
                            x = x->parent_;
                        }
                    } else {
                        child->updated_ = false;
                    }
                }
            }
            return result;
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

        size_t size() const {
            return elements_.size();
        }

        T const & operator [] (size_t index) const {
            ASSERT(index < elements_.size());
            return * dynamic_cast<T*>(elements_[index]);
        }

        T & operator [] (size_t index) {
            ASSERT(index < elements_.size());
            return * dynamic_cast<T*>(elements_[index]);
        }

        JSON toJSON(bool updatedOnly = true) const override {
            ASSERT(updated_ || ! updatedOnly);
            ASSERT(json_ != nullptr);
            JSON result{JSON::Array()};
            result.setComment(json_->comment());
            for (auto i : elements_)
                result.add(i->toJSON(updatedOnly));
            return result;
        }

        T & addElement() {
            T * element = new T{this};
            return * element;
        }

        void erase(JSONConfig const & element) {
            for (size_t i = 0, e = elements_.size(); i < e; ++i) {
                if (elements_[i] == & element) {
                    elements_.erase(elements_.begin() + i);
                    json_->erase(i);
                }
            }
        }

    protected:

        bool update(JSON const & value, std::function<void(JSONError &&)> errorHandler = [](JSONError && e) { throw e; }) override {
            ASSERT(json_ != nullptr);
            *json_ = JSON::Array();
            if (value.kind() != JSON::Kind::Array) {
                errorHandler(CREATE_EXCEPTION(JSONError())  << "Initializing " << name() << " with " << value << ", but array expected");
                return false;
            }
            bool result = false;
            updated_ = true;
            json_->setComment(value.comment());
            // delete previous value
            for (auto i : elements_)
                delete i;
            elements_.clear();
            // update the values
            for (auto i = value.begin(), e = value.end(); i != e; ++i) {
                T * element = new T{this};
                result = JSONConfig::updateOther(element, *i, errorHandler) || result;
            }
            return result;
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
            JSONConfig{parent, name, description, defaultValue},
            initialized_{false} {
        }

        Property(JSONConfig * parent, std::string name, std::string description, std::function<JSON()> defaultValue):
            JSONConfig{parent, name, description, defaultValue},
            initialized_{false} {
        }
            
        /** Typecasts the configuration property to the property value type. 
         */
        T const & operator () () const {
            if (!initialized_) {
                if (std::holds_alternative<JSON>(defaultValue_))
                    const_cast<Property<T>*>(this)->update(std::get<JSON>(defaultValue_));
                else
                    const_cast<Property<T>*>(this)->update(std::get<std::function<JSON()>>(defaultValue_)());
            }
            return value_;
        }

        JSON toJSON(bool updatedOnly = true) const override {
            ASSERT(updated_ || ! updatedOnly);
            ASSERT(json_ != nullptr);
            return *json_;
        }

    protected:

        bool update(JSON const & value, std::function<void(JSONError &&)> errorHandler = [](JSONError && e) { throw e; }) override {
            ASSERT(json_ != nullptr);
            try {
                value_ = JSONConfig::FromJSON<T>(value);
                *json_ = value;
                updated_ = true;
                initialized_ = true;
            } catch (std::exception const & e) {
                errorHandler(CREATE_EXCEPTION(JSONError()) << "Error when parsing JSON value for " << name() << ": " << e.what());
            }
            return false;
        }

        void cmdArgUpdate(char const * value, size_t index) override {
            JSONConfig::cmdArgUpdate(value, index);
        }

        bool cmdArgRequiresValue() const override {
            return JSONConfig::cmdArgRequiresValue();
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

        bool initialized_;
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

    protected:

        /** Initializes the configuration with default values. 
         
            Because default values support lazy initialization, they are only filled in when a configuration is read. For cases where configuration is not read 
         */
        void fillDefaultValues() {
            update(JSON::Object());
        }

    }; // JSONConfig::Root

    /** Configuration root element with command line arguments parsing.
     
        
        */
    class JSONConfig::CmdArgsRoot : public JSONConfig::Root {
    public:

    protected:

        CmdArgsRoot():
            lastArgument_{nullptr},
            defaultArgument_{nullptr} {
        }

        void addArgumentPositional(JSONConfig & config) {
            positionalArguments_.push_back(&config);
        }

        /** Registers given configuration option as a command line argument under the specified aliases. 
         */
        void addArgument(JSONConfig & config, char const * name) {
            addArgument(config, { name });
        }

        void addArgument(JSONConfig & config, std::initializer_list<char const *> aliases) {
            for (auto i : aliases) {
                std::string alias{i};
                if (! keywordArguments_.insert(std::make_pair(alias, & config)).second)
                    ASSERT(false) << "Alias " << alias << " already bound to " << keywordArguments_[alias]->name();
            }
        }

        void setLastArgument(JSONConfig & config) {
            ASSERT(lastArgument_ == nullptr) << "Last argument already set to " << lastArgument_->name();
            lastArgument_ = &config;
        }

        void setDefaultArgument(JSONConfig & config) {
            ASSERT(defaultArgument_ == nullptr) << "Default argument already set to " << defaultArgument_->name();
            defaultArgument_ = &config;
        }

        /** Parses the command line arguments. 
         */
        void parseCommandLine(int argc, char * argv[]) {
            try {
                std::unordered_map<JSONConfig *, size_t> occurences;
                int i = 1;
                parsePositionalArguments(i, argc, argv, occurences);
                parseKeywordArguments(i, argc, argv, occurences);
            } catch (...) {
                positionalArguments_.clear();                    
                keywordArguments_.clear();
                throw;
            }
        }

    private:

        void updateArgument(JSONConfig * arg, char const * value, std::unordered_map<JSONConfig *, size_t> & occurences) {
            auto i = occurences.find(arg);
            if (i == occurences.end())
                i = occurences.insert(std::make_pair(arg, 0)).first;
            arg->cmdArgUpdate(value, i->second);
            i->second += 1;
        }

        void parsePositionalArguments(int & i, int argc, char * argv[], std::unordered_map<JSONConfig *, size_t> & occurences) {
            for (JSONConfig * arg : positionalArguments_) {
                if (i == argc)
                    THROW(ArgumentError()) << "Argument " << arg->name() << " not provided";
                updateArgument(arg, argv[i++], occurences);
                if (lastArgument_ == arg) {
                    while ( i < argc)
                        updateArgument(arg, argv[i++], occurences);
                }
            }
        }

        void parseKeywordArguments(int & i, int argc, char * argv[], std::unordered_map<JSONConfig *, size_t> & occurences) {
            while (i < argc) {
                char const * argValue = nullptr;
                std::string argName{argv[i]};
                auto arg = keywordArguments_.find(argName);
                // if the entire argument is not recognized argument alias, split it by the '='
                if (arg == keywordArguments_.end()) {
                    size_t x = argName.find('=');
                    if (x == std::string::npos) {
                        if (defaultArgument_ != nullptr) {
                            updateArgument(defaultArgument_, argName.c_str(), occurences);
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
                            updateArgument(defaultArgument_, argv[i], occurences);
                            ++i;
                            continue;
                        } else {
                            THROW(ArgumentError()) << "Unknown argument name " << argName;
                        }
                    }
                }
                if (argValue == nullptr && arg->second->cmdArgRequiresValue()) {
                    if (++i == argc)
                        THROW(ArgumentError()) << "Argument " << arg->second->name() << " value not provided";
                    argValue = argv[i];
                }
                // parse the value
                updateArgument(arg->second, argValue, occurences);
                ++i;
                if (lastArgument_ == arg->second) {
                    while (i < argc) 
                        updateArgument(arg->second, argv[i++], occurences);
                }
            }
        }

        /* registered command line arguments */
        std::unordered_map<std::string, JSONConfig *> keywordArguments_;

        std::vector<JSONConfig*> positionalArguments_;

        JSONConfig * lastArgument_;

        JSONConfig * defaultArgument_;

    }; 

    template<>
    inline std::string JSONConfig::FromJSON<std::string>(JSON const & json) {
        if (json.kind() != JSON::Kind::String)
            THROW(JSONError()) << "Expected string, but " << json << " found";
        return json.toString();
    }

    template<>
    inline void JSONConfig::Property<std::string>::cmdArgUpdate(char const * value, size_t index) {
        if (index != 0)
            THROW(JSONError()) << "Value for " << name() << " already provided";
        update(JSON{value});
    }

    template<>
    inline bool JSONConfig::FromJSON<bool>(JSON const & json) {
        if (json.kind() != JSON::Kind::Boolean)
            THROW(JSONError()) << "Expected bool, but " << json << " found";
        return json.toBool();
    }

    template<>
    inline void JSONConfig::Property<bool>::cmdArgUpdate(char const * value, size_t index) {
        if (index != 0)
            THROW(JSONError()) << "Value for " << name() << " already provided";
        if (value == nullptr)
            update(JSON{true});
        else
            update(JSON{value});
    }

    template<>
    inline bool JSONConfig::Property<bool>::cmdArgRequiresValue() const {
        return false;
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
    
HELPERS_NAMESPACE_END