#pragma once

#include "helpers.h"
#include "string.h"

HELPERS_NAMESPACE_BEGIN

    /** Program version wrapper. 
     
        Although the version numbers look like an extension of semantic versioning, it is not intended to be which is why the last digit is not patch, but build. 

        Version support equality comaprison and ordering. 
     */
    class Version {
    public:
        unsigned major = 0;
        unsigned minor = 0;
        unsigned build = 0;

        /** Creates version 0.0.0. 
         */
        Version() = default;

        /** Creates version from given major, minor and build version. 
         */
        Version(unsigned major, unsigned minor, unsigned build):
            major{major},
            minor{minor},
            build{build} {
        }

        /** Creates version by parsing a string version object. 
         */
        explicit Version(std::string_view from) : Version{} {
            std::vector<std::string_view> elements = Split(from, ".");
            if (elements.size() >= 1) {
                major = std::stoul(std::string{elements[0]});
                if (elements.size() >= 2) {
                    minor = std::stoul(std::string{elements[1]});
                    if (elements.size() == 3)
                        build = std::stoul(std::string{elements[2]});
                    else if (elements.size() > 3)
                        throw std::invalid_argument{"Invalid version argument, expected MAJ.MIN.BLD"};
                }
            }
        }

        Version & operator = (Version const & other) = default;

        Version & operator = (std::string_view from) {
            *this = Version{from};
            return *this;
        }

        bool operator == (Version const & other) const {
            return major == other.major && minor == other.minor && build == other.build;
        }

        bool operator != (Version const & other) const {
            return build != other.build || minor != other.minor || major != other.major;
        }

        bool operator < (Version const & other) const {
            if (major < other.major) {
                return true;
            } else if (major == other.major) {
                if (minor < other.minor)
                    return true;
                else if (minor == other.minor)
                    return build < other.build; 
            }
            return false;
        }

        bool operator <= (Version const & other) const {
            if (major < other.major) {
                return true;
            } else if (major == other.major) {
                if (minor < other.minor)
                    return true;
                else if (minor == other.minor)
                    return build <= other.build; 
            }
            return false;
        }

        bool operator > (Version const & other) const {
            if (major > other.major) {
                return true;
            } else if (major == other.major) {
                if (minor > other.minor)
                    return true;
                else if (minor == other.minor)
                    return build > other.build; 
            }
            return false;
        }

        bool operator >= (Version const & other) const {
            if (major > other.major) {
                return true;
            } else if (major == other.major) {
                if (minor > other.minor)
                    return true;
                else if (minor == other.minor)
                    return build >= other.build; 
            }
            return false;
        }

        friend std::ostream & operator << (std::ostream & s, Version const & v) {
            s << v.major << "." << v.minor << "." << v.build;
            return s;
        }

    }; 

    inline bool CheckVersion(int argc, char ** argv, std::function<void()> versionPrinter) {
        if (argc == 2 && strncmp(argv[1], "--version", 10) == 0) {
            versionPrinter();
            return true;
        }
        return false;
    }

HELPERS_NAMESPACE_END
