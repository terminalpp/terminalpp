#pragma once

#include <string>
#include <iomanip>
#include <vector>
#include <unordered_map>

#include "helpers.h"
#include "string.h"

HELPERS_NAMESPACE_BEGIN

    class JSONError : public Exception {
    public:
        JSONError() = default;

        JSONError(unsigned line, unsigned col) {
            what_ = STR("Parser error at [" << line << "," << col << "]:");
        }
    }; // JSONError

    /* A simple class for storing, serializing and deserializing JSON values. 
     */
    class JSON {
    public:

        /** Iterator to the array and object values. 
         */
        class ConstIterator {
        public:

            std::string const & name() const {
                if (json_.kind_ != Kind::Object)
                    THROW(JSONError()) << "Cannot get name of JSON array iterator";
                return iObject_->first;
            }

            size_t index() const {
                if (json_.kind_ != Kind::Array)
                    THROW(JSONError()) << "Cannot get name of JSON array iterator";
                return iArray_ - json_.valueArray_.begin();
            }

            JSON const & operator * () const {
                switch(json_.kind_) {
                    case Kind::Array:
                        ASSERT(iArray_ != json_.valueArray_.end());
                        return *(*iArray_);
                    case Kind::Object:
                        ASSERT(iObject_ != json_.valueObject_.end());
                        return *(iObject_->second);
                    default:
                        UNREACHABLE;
                }
            }

            JSON const * operator -> () const {
                switch(json_.kind_) {
                    case Kind::Array:
                        ASSERT(iArray_ != json_.valueArray_.end());
                        return *iArray_;
                    case Kind::Object:
                        ASSERT(iObject_ != json_.valueObject_.end());
                        return iObject_->second;
                    default:
                        UNREACHABLE;
                }
            }

            ConstIterator & operator ++ () {
                switch (json_.kind_) {
                    case Kind::Array:
                        ++iArray_;
                        break;
                    case Kind::Object:
                        ++iObject_;
                        break;
                    default:
                        UNREACHABLE;
                }
                return *this;
            }

            ConstIterator operator ++ (int) {
                ConstIterator result(*this);
                ++(*this);
                return result;
            }

            bool operator == (ConstIterator const & other) const {
                if (&json_ != &other.json_)
                    return false;
                switch(json_.kind_) {
                    case Kind::Array:
                        return iArray_ == other.iArray_;
                    case Kind::Object:
                        return iObject_ == other.iObject_;
                        break;
                    default:
                        UNREACHABLE;
                }
            }

            bool operator != (ConstIterator const & other) const {
                if (&json_ != &other.json_)
                    return true;
                switch(json_.kind_) {
                    case Kind::Array:
                        return iArray_ != other.iArray_;
                    case Kind::Object:
                        return iObject_ != other.iObject_;
                        break;
                    default:
                        UNREACHABLE;
                }
            }
            
        private:
            friend class JSON;

            ConstIterator(JSON const & json, std::vector<JSON*>::const_iterator const & it):
                json_(json),
                iArray_(it) {
            }

            ConstIterator(JSON const & json, std::unordered_map<std::string, JSON*>::const_iterator const & it):
                json_(json),
                iObject_(it) {
            }

            JSON const & json_;
            std::vector<JSON *>::const_iterator iArray_;
            std::unordered_map<std::string, JSON *>::const_iterator iObject_;
        };

        /** Iterator to the array and object values. 
         */
        class Iterator {
        public:

            std::string const & name() const {
                if (json_.kind_ != Kind::Object)
                    THROW(JSONError()) << "Cannot get name of JSON array iterator";
                return iObject_->first;
            }

            size_t index() const {
                if (json_.kind_ != Kind::Array)
                    THROW(JSONError()) << "Cannot get name of JSON array iterator";
                return iArray_ - json_.valueArray_.begin();
            }

            JSON & operator * () {
                switch(json_.kind_) {
                    case Kind::Array:
                        ASSERT(iArray_ != json_.valueArray_.end());
                        return *(*iArray_);
                    case Kind::Object:
                        ASSERT(iObject_ != json_.valueObject_.end());
                        return *(iObject_->second);
                    default:
                        UNREACHABLE;
                }
            }

            JSON * operator -> () {
                switch(json_.kind_) {
                    case Kind::Array:
                        ASSERT(iArray_ != json_.valueArray_.end());
                        return *iArray_;
                    case Kind::Object:
                        ASSERT(iObject_ != json_.valueObject_.end());
                        return iObject_->second;
                    default:
                        UNREACHABLE;
                }
            }

            Iterator & operator ++ () {
                switch (json_.kind_) {
                    case Kind::Array:
                        ++iArray_;
                        break;
                    case Kind::Object:
                        ++iObject_;
                        break;
                    default:
                        UNREACHABLE;
                }
                return *this;
            }

            Iterator operator ++ (int) {
                Iterator result(*this);
                ++(*this);
                return result;
            }

            bool operator == (Iterator const & other) const {
                if (&json_ != &other.json_)
                    return false;
                switch(json_.kind_) {
                    case Kind::Array:
                        return iArray_ == other.iArray_;
                    case Kind::Object:
                        return iObject_ == other.iObject_;
                        break;
                    default:
                        UNREACHABLE;
                }
            }

            bool operator != (Iterator const & other) const {
                if (&json_ != &other.json_)
                    return true;
                switch(json_.kind_) {
                    case Kind::Array:
                        return iArray_ != other.iArray_;
                    case Kind::Object:
                        return iObject_ != other.iObject_;
                        break;
                    default:
                        UNREACHABLE;
                }
            }
            
        private:
            friend class JSON;

            Iterator(JSON const & json, std::vector<JSON*>::iterator const & it):
                json_(json),
                iArray_(it) {
            }

            Iterator(JSON const & json, std::unordered_map<std::string, JSON*>::iterator const & it):
                json_(json),
                iObject_(it) {
            }

            JSON const & json_;
            std::vector<JSON *>::iterator iArray_;
            std::unordered_map<std::string, JSON *>::iterator iObject_;
        };


        /** Parses the given input stream into a JSON object and returns it. 

            The following grammar is supported:

            JSON := [ COMMENT ] ELEMENT
            ELEMENT := null | true | false | int | double | STR | ARRAY | OBJECT
            STR := double quoted string
            COMMENT := C/Javascript single or multi-line style comment
            ARRAY := '[' [ JSON { ',' JSON } ] ']'
            OBJECT := '{' [ [ COMMENT ] STR ':' ELEMENT { ',' [COMMENT] STR ':' ELEMENT } ] }
         */
        static JSON Parse(std::string const & from) {
            std::stringstream ss(from);
            return Parse(ss);
        }

        static JSON Parse(std::istream & s);

        /** Returns an empty JSON object with no properties. 
         */
        static JSON Object() {
            return JSON{Kind::Object};
        }

        /** Returns an empty JSON array with no items. 
         */
        static JSON Array() {
            return JSON{Kind::Array};
        }

        static JSON Null() {
            return JSON{Kind::Null};
        }

        enum class Kind {
            Null,
            Boolean,
            Integer,
            Double,
            String,
            Array,
            Object,
        };

        explicit JSON(Kind kind = Kind::Null):
            kind_(kind),
            valueInt_(0) {
            switch (kind) {
                case Kind::Null:
                case Kind::Integer:
                    break; // already dealt with
                case Kind::Boolean:
                    valueBool_ = false;
                    break;
                case Kind::Double:
                    valueDouble_ = 0;
                    break;
                case Kind::String:
                    new (&valueStr_) std::string();
                    break;
                case Kind::Array:
                    new (&valueArray_) std::vector<JSON *>();
                    break;
                case Kind::Object:
                    new (&valueObject_) std::unordered_map<std::string, JSON *>();
                    break;
            }
        }

        explicit JSON(std::nullptr_t):
            kind_(Kind::Null),
            valueInt_(0) {
        }

        explicit JSON(bool value):
            kind_(Kind::Boolean),
            valueBool_(value ? 1 : 0) {
        }

        explicit JSON(int value):
            kind_(Kind::Integer),
            valueInt_(value) {
        }

        explicit JSON(unsigned value):
            kind_(Kind::Integer),
            valueInt_(static_cast<int>(value)) {
        }

        explicit JSON(double value):
            kind_(Kind::Double),
            valueDouble_(value) {
        }

        explicit JSON(char const * value):
            kind_(Kind::String),
            valueStr_(value) {
        }

        explicit JSON(std::string const & value):
            kind_(Kind::String),
            valueStr_(value) {
        }

        JSON(JSON const & other):
            kind_(other.kind_),
            comment_(other.comment_),
            valueInt_(other.valueInt_) {
            switch (kind_) {
                case Kind::Null:
                case Kind::Integer:
                    break; // already dealt with
                case Kind::Boolean:
                    valueBool_ = other.valueBool_;
                    break;
                case Kind::Double:
                    valueDouble_ = other.valueDouble_;
                    break;
                case Kind::String:
                    new (&valueStr_) std::string(other.valueStr_);
                    break;
                case Kind::Array:
                    new (&valueArray_) std::vector<JSON *>();
                    for (JSON * i : other.valueArray_)
                        valueArray_.push_back(new JSON(*i));
                    break;
                case Kind::Object:
                    new (&valueObject_) std::unordered_map<std::string, JSON *>();
                    for (auto i : other.valueObject_)
                        valueObject_.insert(std::make_pair(i.first, new JSON(*i.second)));
                    break;
            }
        }

        JSON(JSON && other):
            kind_(other.kind_),
            comment_(std::move(other.comment_)),
            valueInt_(other.valueInt_) {
            switch (kind_) {
                case Kind::Null:
                case Kind::Integer:
                    break; // already dealt with
                case Kind::Boolean:
                    valueBool_ = other.valueBool_;
                    break;
                case Kind::Double:
                    valueDouble_ = other.valueDouble_;
                    break;
                case Kind::String:
                    new (&valueStr_) std::string(std::move(other.valueStr_));
                    break;
                case Kind::Array:
                    new (&valueArray_) std::vector<JSON *>(std::move(other.valueArray_));
                    break;
                case Kind::Object:
                    new (&valueObject_) std::unordered_map<std::string, JSON *>(std::move(other.valueObject_));
                    break;
            }
        }

        ~JSON() {
            destroy();
        }

        JSON & operator = (JSON const & other) {
            if (this == &other)
                return *this;
            if (kind_ != other.kind_) {
                destroy();
                kind_ = other.kind_;
            }
            comment_ = other.comment_;
            switch (kind_) {
                case Kind::Null:
                case Kind::Integer:
                    valueInt_ = other.valueInt_;
                    break; 
                case Kind::Boolean:
                    valueBool_ = other.valueBool_;
                    break;
                case Kind::Double:
                    valueDouble_ = other.valueDouble_;
                    break;
                case Kind::String:
                    new (&valueStr_) std::string(other.valueStr_);
                    break;
                case Kind::Array:
                    new (&valueArray_) std::vector<JSON *>{};
                    for (JSON * i : other.valueArray_)
                        valueArray_.push_back(new JSON(*i));
                    break;
                case Kind::Object:
                    new (&valueObject_) std::unordered_map<std::string, JSON *>{};
                    for (auto i : other.valueObject_)
                        valueObject_.insert(std::make_pair(i.first, new JSON(*i.second)));
                    break;
            }
            return *this;
        }

        JSON & operator = (JSON && other) {
            if (this == &other)
                return *this;
            if (kind_ != other.kind_) {
                destroy();
                kind_ = other.kind_;
            }
            comment_ = std::move(other.comment_);
            switch (kind_) {
                case Kind::Null:
                case Kind::Integer:
                    valueInt_ = other.valueInt_;
                    break; 
                case Kind::Boolean:
                    valueBool_ = other.valueBool_;
                    break;
                case Kind::Double:
                    valueDouble_ = other.valueDouble_;
                    break;
                case Kind::String:
                    new (&valueStr_) std::string(std::move(other.valueStr_));
                    break;
                case Kind::Array:
                    new (&valueArray_) std::vector<JSON *>(std::move(other.valueArray_));
                    break;
                case Kind::Object:
                    new (&valueObject_) std::unordered_map<std::string, JSON *>(std::move(other.valueObject_));
                    break;
            }
            return *this;
        }

        JSON & operator = (std::nullptr_t) {
            if (kind_ != Kind::Null)
                destroy();
            kind_ = Kind::Null;
            valueInt_ = 0;;
            return *this;
        }


        JSON & operator = (bool value) {
            if (kind_ != Kind::Boolean)
                destroy();
            kind_ = Kind::Boolean;
            valueBool_ = value;
            return *this;
        }

        JSON & operator = (int value) {
            if (kind_ != Kind::Integer)
                destroy();
            kind_ = Kind::Integer;
            valueInt_ = value;
            return *this;
        }

        JSON & operator = (double value) {
            if (kind_ != Kind::Double)
                destroy();
            kind_ = Kind::Double;
            valueDouble_ = value;
            return *this;
        }

        JSON & operator = (std::string const & value) {
            if (kind_ != Kind::String)
                destroy();
            kind_ = Kind::String;
            new (&valueStr_) std::string(value);
            return *this;
        }

        JSON & operator = (char const * value) {
            if (kind_ != Kind::String)
                destroy();
            kind_ = Kind::String;
            new (&valueStr_) std::string(value);
            return *this;
        }

        Kind kind() const {
            return kind_;
        }

        bool isNull() const {
            return kind_ == Kind::Null;
        }

        bool isBool() const {
            return kind_ == Kind::Boolean;
        }

        bool hasKey(std::string const & key) const {
            if (kind_ != Kind::Object)
                THROW(JSONError()) << "Cannot look for key in JSON element of type " << kind_;
            return valueObject_.find(key) != valueObject_.end();
        }

        size_t numElements() const {
            switch (kind_) {
                case Kind::Array:
                    return valueArray_.size();
                case Kind::Object:
                    return valueObject_.size();
                default:
                    THROW(JSONError()) << "Unable to get size of JSON element of type " << kind_;
            }
        }

        /** Returns true if the element is empty. 
         
            Only makes sense for strings, arrays and objects. Null is always empty and other element types are never empty. 
         */
        bool empty() const {
            switch (kind_) {
                case Kind::Null:
                    return true;
                case Kind::String:
                    return valueStr_.empty();
                case Kind::Array:
                    return valueArray_.empty();
                case Kind::Object:
                    return valueObject_.empty();
                default:
                    return false;
            }
        }

        std::string const & comment() const {
            return comment_;
        }

        JSON & setComment(std::string const & value) {
            comment_ = value;
            return *this;
        }

        bool toBool() const {
            if (kind_ != Kind::Boolean)
                THROW(JSONError()) << "Cannot obtain boolean value from element holding " << kind_;
            return valueBool_;
        }

        int toInt() const {
            if (kind_ != Kind::Integer)
                THROW(JSONError()) << "Cannot obtain integer value from element holding " << kind_;
            return valueInt_;
        }

        unsigned toUnsigned() const {
            if (kind_ != Kind::Integer)
                THROW(JSONError()) << "Cannot obtain integer value from element holding " << kind_;
            if (valueInt_ < 0)
                THROW(JSONError()) << "Unsigned value expected but " << valueInt_ << " found";
            return valueInt_;
        }

        double toDouble() const {
            if (kind_ != Kind::Double)
                THROW(JSONError()) << "Cannot obtain double value from element holding " << kind_;
            return valueDouble_;
        }

        std::string const & toString() const {
            if (kind_ != Kind::String)
                THROW(JSONError()) << "Cannot obtain string value from element holding " << kind_;
            return valueStr_;
        }

        operator bool() const {
            return toBool();
        } 

        operator int() const {
            return toInt();
        } 

        operator unsigned() const {
            return toUnsigned();
        }

        operator double () const {
            return toDouble();
        } 

        operator std::string const & () const {
            return toString();
        } 

        JSON const & operator [] (size_t index) const {
            if (kind_ != Kind::Array)
                THROW(JSONError()) << "Cannot index JSON element holding " << kind_;
            if (index >= valueArray_.size())
                THROW(JSONError()) << "Index " << index << " too large (available: " << valueArray_.size() << ")";
            return *valueArray_[index];

        }

        JSON & operator [] (size_t index) {
            if (kind_ != Kind::Array)
                THROW(JSONError()) << "Cannot index JSON element holding " << kind_;
            while (index >= valueArray_.size())
                valueArray_.push_back(new JSON());
            return *valueArray_[index];
        }

        JSON const & operator [] (std::string const & index) const {
            if (kind_ != Kind::Object)
                THROW(JSONError()) << "Cannot obtain property of JSON element holding " << kind_;
            auto i = valueObject_.find(index);
            if (i == valueObject_.end())
                THROW(JSONError()) << "Key " << index << " does not exist";
            return *(i->second);
        }

        JSON & operator [] (std::string const & index) {
            if (kind_ != Kind::Object)
                THROW(JSONError()) << "Cannot obtain property of JSON element holding " << kind_;
            auto i = valueObject_.find(index);
            if (i == valueObject_.end())
                i = valueObject_.insert(std::make_pair(index, new JSON())).first;
            return *(i->second);
        }

        JSON const & operator [] (char const * index) const {
            if (kind_ != Kind::Object)
                THROW(JSONError()) << "Cannot obtain property of JSON element holding " << kind_;
            std::string idx{index};
            auto i = valueObject_.find(idx);
            if (i == valueObject_.end())
                THROW(JSONError()) << "Key " << idx << " does not exist";
            return *(i->second);
        }

        JSON & operator [] (char const * index) {
            if (kind_ != Kind::Object)
                THROW(JSONError()) << "Cannot obtain property of JSON element holding " << kind_;
            std::string idx{index};
            auto i = valueObject_.find(idx);
            if (i == valueObject_.end())
                i = valueObject_.insert(std::make_pair(idx, new JSON())).first;
            return *(i->second);
        }

        JSON & add(JSON const & what) {
            if (kind_ != Kind::Array)
                THROW(JSONError()) << "Cannot add array elemnt to element holding " << kind_;
            JSON * result = new JSON{what};
            valueArray_.push_back(result);
            return * result;
        } 

        JSON & add(JSON && what) {
            if (kind_ != Kind::Array)
                THROW(JSONError()) << "Cannot add array elemnt to element holding " << kind_;
            JSON * result = new JSON{std::move(what)};
            valueArray_.push_back(result);
            return * result;
        }

        JSON & add(std::string const & key, JSON const & value) {
            if (kind_ != Kind::Object)
                THROW(JSONError()) << "Cannot add member elemnt to element holding " << kind_;
            JSON * result = new JSON{value};
            valueObject_.insert(std::make_pair(key, result));
            return * result;
        } 

        JSON & add(std::string const & key, JSON && value) {
            if (kind_ != Kind::Object)
                THROW(JSONError()) << "Cannot add member elemnt to element holding " << kind_;
            JSON * element = new JSON{std::move(value)};
            if (! valueObject_.insert(std::make_pair(key, element)).second) {
                delete element;
                THROW(JSONError()) << "Value " << key << " already exists";
            }
            return * element;
        }

        void erase(std::string const & key) {
            if (kind_ != Kind::Object)
                THROW(JSONError()) << "Only objects can erase their members, but " << kind_ << " found";
            auto i = valueObject_.find(key);
            if (i != valueObject_.end()) {
                delete i->second;
                valueObject_.erase(i);
            }
        }

        void erase(size_t index) {
            if (kind_ != Kind::Array)
                THROW(JSONError()) << "Cannot add array elemnt to element holding " << kind_;
            valueArray_.erase(valueArray_.begin() + index);
        }

        void clear() {
            switch(kind_) {
                case Kind::Array:
                    for (JSON * i : valueArray_)
                        delete i;
                    valueArray_.clear();
                    break;
                case Kind::Object:
                    for (auto i : valueObject_)
                        delete i.second;
                    valueObject_.clear();
                    break;
                default:
                    THROW(JSONError()) << "Iterator only available for arrays and objects, not for " << kind_;
            }
        }

        ConstIterator begin() const {
            switch(kind_) {
                case Kind::Array:
                    return ConstIterator(*this,valueArray_.begin());
                case Kind::Object:
                    return ConstIterator(*this,valueObject_.begin());
                default:
                    THROW(JSONError()) << "Iterator only available for arrays and objects, not for " << kind_;
            }
        }

        ConstIterator end() const {
            switch(kind_) {
                case Kind::Array:
                    return ConstIterator(*this,valueArray_.end());
                case Kind::Object:
                    return ConstIterator(*this,valueObject_.end());
                default:
                    THROW(JSONError()) << "Iterator only available for arrays and objects, not for " << kind_;
            }
        }

        JSON * get(std::initializer_list<std::string> path) {
            return get(path.begin(), path.end());
        }

        Iterator begin() {
            switch(kind_) {
                case Kind::Array:
                    return Iterator(*this,valueArray_.begin());
                case Kind::Object:
                    return Iterator(*this,valueObject_.begin());
                default:
                    THROW(JSONError()) << "Iterator only available for arrays and objects, not for " << kind_;
            }
        }

        Iterator end() {
            switch(kind_) {
                case Kind::Array:
                    return Iterator(*this,valueArray_.end());
                case Kind::Object:
                    return Iterator(*this,valueObject_.end());
                default:
                    THROW(JSONError()) << "Iterator only available for arrays and objects, not for " << kind_;
            }
        }

        bool operator == (JSON const & other) const {
            if (kind_ != other.kind_)
                return false;
            switch (kind_) {
                case Kind::Null:
                    return true;
                case Kind::Boolean:
                    return valueBool_ == other.valueBool_;
                case Kind::Integer:
                    return valueInt_ == other.valueInt_;
                case Kind::Double:
                    return valueDouble_ == other.valueDouble_;
                case Kind::String:
                    return valueStr_ == other.valueStr_;
                case Kind::Array:
                    if (valueArray_.size() != other.valueArray_.size())
                        return false;
                    for (size_t i = 0, e = valueArray_.size(); i != e; ++i)
                        if (*valueArray_[i] != *other.valueArray_[i])
                            return false;
                    return true;
                case Kind::Object:
                    if (valueObject_.size() != other.valueObject_.size())
                        return false;
                    for (auto i : valueObject_) {
                        auto j = other.valueObject_.find(i.first);
                        if (j == other.valueObject_.end())
                            return false;
                        if (*i.second != *j->second)
                            return false;
                    }
                    return true;
                default:
                    UNREACHABLE;
            }
        }

        bool operator != (JSON const & other) const {
            if (kind_ != other.kind_)
                return true;
            switch (kind_) {
                case Kind::Null:
                    return false;
                case Kind::Boolean:
                    return valueBool_ != other.valueBool_;
                case Kind::Integer:
                    return valueInt_ != other.valueInt_;
                case Kind::Double:
                    return valueDouble_ != other.valueDouble_;
                case Kind::String:
                    return valueStr_ != other.valueStr_;
                case Kind::Array:
                    if (valueArray_.size() != other.valueArray_.size())
                        return true;
                    for (size_t i = 0, e = valueArray_.size(); i != e; ++i)
                        if (*valueArray_[i] != *other.valueArray_[i])
                            return true;
                    return false;
                case Kind::Object:
                    if (valueObject_.size() != other.valueObject_.size())
                        return true;
                    for (auto i : valueObject_) {
                        auto j = other.valueObject_.find(i.first);
                        if (j == other.valueObject_.end())
                            return true;
                        if (*i.second != *j->second)
                            return true;
                    }
                    return false;
                default:
                    UNREACHABLE;
            }
        }

        bool operator == (bool other) const {
            return kind_ == Kind::Boolean && valueBool_ == other;
        }

        bool operator == (int other) const {
            return kind_ == Kind::Integer && valueInt_ == other;
        }

        bool operator == (double other) const {
            return kind_ == Kind::String && valueDouble_ == other;
        }

        bool operator == (std::string const & other) const {
            return kind_ == Kind::String && valueStr_ == other;
        }

        void writeTo(std::ostream & s, unsigned tabWidth = 4) const {
            writeComment(s, tabWidth, 0);
            writeTo(s, tabWidth, 0);
        }

        friend std::ostream & operator << (std::ostream & s, Kind kind) {
            switch (kind) {
                case Kind::Null:
                    s << "null";
                    break;
                case Kind::Boolean:
                    s << "boolean";
                    break;
                case Kind::Integer:
                    s << "integer";
                    break;
                case Kind::Double:
                    s << "double";
                    break;
                case Kind::String:
                    s << "string";
                    break;
                case Kind::Array:
                    s << "array";
                    break;
                case Kind::Object:
                    s << "object";
                    break;
                default:
                    UNREACHABLE;
            }
            return s;
        }

        friend std::ostream & operator << (std::ostream & s, JSON const & json) {
            json.writeTo(s);
            return s;
        }


    private:
        friend class Iterator;
        friend class ConstIterator;
        class Parser;


        void destroy() {
            switch (kind_) {
                case Kind::Null:
                case Kind::Boolean:
                case Kind::Integer:
                case Kind::Double:
                    break; // no need to destroy pods
                case Kind::String:
                    valueStr_.~basic_string();
                    break;
                case Kind::Array:
                    for (auto i : valueArray_) 
                        delete i;
                    valueArray_.~vector();
                    break;
                case Kind::Object:
                    for (auto i : valueObject_) 
                        delete i.second;
                    valueObject_.~unordered_map();
                    break;
            }
        }

        void writeComment(std::ostream & s, unsigned tabWidth, unsigned offset) const {
            if (comment_.empty())
                return;
            std::vector<std::string_view> lines(Split(comment_, "\n"));
            auto i = lines.begin();
            s << std::setw(offset) << "" << "/* " << *i << std::endl;
            ++i;
            offset += tabWidth;
            while (i != lines.end()) {
                s << std::setw(offset) << "" << (*i) << std::endl;
                ++i;
            }
            offset -= tabWidth;
            s << std::setw(offset) << "" << " */" << std::endl;
        }

        void writeTo(std::ostream & s, unsigned tabWidth, unsigned offset) const {
            switch (kind_) {
                case Kind::Null:
                    s << "null";
                    break;
                case Kind::Boolean:
                    s << (valueBool_ ? "true" : "false");
                    break;
                case Kind::Integer:
                    s << valueInt_;
                    break;
                case Kind::Double:
                    s << valueDouble_;
                    break;
                case Kind::String:
                    s << Quote(valueStr_);
                    break;
                case Kind::Array:
                    s << "[" << std::endl;
                    offset += tabWidth;
                    for (auto i = valueArray_.begin(), e = valueArray_.end(); i != e;) {
                        (*i)->writeComment(s,tabWidth, offset);
                        s << std::setw(offset) << "";
                        (*i)->writeTo(s, tabWidth, offset);
                        ++i;
                        if (i != e)
                            s << ",";
                        s << std::endl;
                    }
                    offset -= tabWidth;
                    s << std::setw(offset) << "" << "]";
                    break;
                case Kind::Object:
                    s << "{" << std::endl;
                    offset += tabWidth;
                    for (auto i = valueObject_.begin(), e = valueObject_.end(); i != e;) {
                        i->second->writeComment(s,tabWidth, offset);
                        s << std::setw(offset) << "" << Quote(i->first) << " : ";
                        i->second->writeTo(s, tabWidth, offset);
                        ++i;
                        if (i != e)
                            s << ",";
                        s << std::endl;
                    }
                    offset -= tabWidth;
                    s << std::setw(offset) << "" << "}";
                    break;
            }
        }

        JSON * get(std::string const * begin, std::string const * end) {
            if (begin == end)
                return this;
            if (kind_ != Kind::Object)
                return nullptr;
            auto i = valueObject_.find(*begin);
            if (i == valueObject_.end())
                return nullptr;
            return i->second->get(begin + 1, end);
        }

        Kind kind_;

        std::string comment_;

        union {
            int valueInt_;
            bool valueBool_;
            double valueDouble_;
            std::string valueStr_;
            std::vector<JSON *> valueArray_;
            std::unordered_map<std::string, JSON *> valueObject_;
        };

    }; // JSON

    // parser
    
    class JSON::Parser {
    public:

        Parser(std::istream & s):
            line_(1),
            col_(1),
            input_(s) {
        }

        /** JSON := [ COMMENT ] ELEMENT
         */
        JSON parseJSON() {
            skipWhitespace();
            if (top() == '/') {
                std::string comment = parseComment();
                JSON result = parseElement();
                result.setComment(comment);
                return result;
            } else {
                return parseElement();
            }
        }

    private:

        friend class JSON;

        /** ELEMENT := null | true | false | int | double | STR | ARRAY | OBJECT
         */
        JSON parseElement() {
            skipWhitespace();
            switch (top()) {
                case 'n':
                    pop("null");
                    return JSON(Kind::Null);
                    break;
                case 't':
                    pop("true");
                    return JSON(true);
                case 'f':
                    pop("false");
                    return JSON(false);
                case '\"':
                    return JSON(parseStr());
                case '[':
                    return parseArray();
                case '{':
                    return parseObject();
                case '-':
                    // fallthrough for number
                default:
                    // it's number
                    return parseNumber();
            }
        }

        /** Parses integer or double number. 
         */
        JSON parseNumber() {
            int sign = condPop('-') ? -1 : 1;
            if (! IsDecimalDigit(top()))
                THROW(JSONError(line_, col_)) << "Expected number, bool, null, string, array, or object but " << top() << " found";
            if (sign == 1 && condPop('0') && condPop('x'))
                return parseHexadecimalInteger();
            int value = 0;
            while (IsDecimalDigit(top())) {
                value = value * 10 + DecCharToNumber(pop());
            };
            if (condPop('.')) {
                double result = value;
                double n = 10.0;
                while (IsDecimalDigit(top())) {
                    result = result + DecCharToNumber(pop()) / n;
                    n *= 10;
                }
                return JSON(result * sign);
            } else {
                return JSON(value * sign);
            }
        }

        JSON parseHexadecimalInteger() {
            // the 0x has already been parsed
            int value = 0;
            if (!IsHexadecimalDigit(top()))
                THROW(JSONError(line_, col_)) << "Expected hexadecimal number but " << top() << " found";
            do {
                value = value * 16 + HexCharToNumber(pop());
            } while (IsHexadecimalDigit(top()));
            return JSON(value);
        }

        /** STR := double quoted string
         */
        std::string parseStr() {
            unsigned l = line_;
            unsigned c = col_;
            std::stringstream result;
            pop('\"');
            while (true) {
                if (input_.eof())
                    THROW(JSONError(l, c)) << "Unterminated string";
                char t = top();
                if ( t == '"') {
                    pop();
                    break;
                }
                if (t == '\\') {
                    pop();
                    switch (top()) {
                        case '\\':
                        case '\'':
                        case '"':
                            result << pop();
                            continue;
                        case '\n':
                            pop();
                            continue;
                        case 'n':
                            result << '\n';
                            pop();
                            continue;
                        case '\t':
                            result << '\t';
                            pop();
                            continue;
                        default:
                            THROW(JSONError(line_, col_)) << "Invalid escape sequence " << top();
                    }
                } else {
                    result << pop();
                }
            }
            return result.str();
        }

        /** COMMENT := C/Javascript single or multi-line style comment
         */
        std::string parseComment() {
            pop('/');
            std::stringstream result;
            switch (top()) {
                case '/': {
                    pop();
                    while (! input_.eof() && ! condPop('\n'))
                        result << pop();
                    break;
                }
                case '*': {
                    pop();
                    while (! input_.eof()) {
                        if (top() == '*') {
                            pop();
                            if (condPop('/'))
                                break;
                            result << '*';
                        } else {
                            result << pop();
                        }
                    }
                    break;
                }
                default:
                    THROW(JSONError(line_, col_)) << "Invalid comment detected";
            }
            return Trim(result.str());
        }

        /** ARRAY := '[' [ JSON { ',' JSON } ] ']'
         */
        JSON parseArray() {
            pop('[');
            JSON result(Kind::Array);
            skipWhitespace();
            if (top() != ']') {
                result.add(parseJSON());
                skipWhitespace();
                while (condPop(',')) {
                    skipWhitespace();
                    result.add(parseJSON());
                    skipWhitespace();
                }
            }
            pop(']');
            return result;
        }

        void parseObjectElement(JSON & result) {
            std::string comment;
            if (top() == '/') {
                comment = parseComment();
                skipWhitespace();
            }
            unsigned l = line_;
            unsigned c = col_;
            std::string key = parseStr();
            if (result.hasKey(key))
                THROW(JSONError(l, c)) << "Key " << key << " already exists";
            skipWhitespace();
            pop(':');
            skipWhitespace();
            JSON value = parseElement();
            value.setComment(comment);
            result.add(key, std::move(value));
        }

        /** OBJECT := '{' [ [ COMMENT ] STR ':' ELEMENT { ',' [COMMENT] STR ':' ELEMENT } ] }
         */
        JSON parseObject() {
            pop('{');
            JSON result(Kind::Object);
            skipWhitespace();
            if (top() != '}') {
                parseObjectElement(result);
                while (condPop(',')) {
                    skipWhitespace();
                    parseObjectElement(result);
                    skipWhitespace();
                }
            }
            skipWhitespace();
            pop('}');
            return result;
        }

        char top() const {
            if (input_.eof())
                return 0;
            return static_cast<char>(input_.peek());
        }

        char pop() {
            char x = top();
            if (input_.get() == '\n') {
                ++line_;
                col_ = 1;
            } else if (x >= 0) { // beginning of a character in UTF8
                ++col_;
            } 
            return x;
        }

        char pop(char what) {
            char x = top();
            if (x != what)
                THROW(JSONError(line_, col_)) << "Expected " << what << ", but " << x << " found";
            return pop();
        }

        void pop(char const * what) {
            char const * w = what;
            unsigned l = line_;
            unsigned c = col_;
            while (*w != 0) {
                if (input_.eof())
                    THROW(JSONError(l, c)) << "Expected " << what << ", but EOF found";
                pop(*w);
                ++w;
            }
        }

        bool condPop(char what) {
            if (top() == what) {
                pop();
                return true;
            } else {
                return false;
            }
        }

        void skipWhitespace() {
            while(IsWhitespace(top()))
                pop();
        }

        unsigned line_;
        unsigned col_;
        std::istream & input_;

    }; // JSON::Parser

    inline JSON JSON::Parse(std::istream & s) {
        Parser p(s);
        JSON result = p.parseJSON();
        p.skipWhitespace();
        if (! s.eof())
            THROW(JSONError(p.line_, p.col_)) << "Unparsed contents";
        return result;
    }


HELPERS_NAMESPACE_END
