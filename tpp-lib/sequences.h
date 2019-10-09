#pragma once

#include <string>

namespace tpp {

    class Sequence {
    public:
        static constexpr int CAPABILITIES = 0;
        static constexpr int CREATE_FILE = 1;
        static constexpr int SEND = 2;
        static constexpr int OPEN = 3;
    };

    /** Returns the t++ capabilities identifier as supported by this implementation. 
     
        For now only returns version 0. In the future this should be parametrized by various capabilities that may or may not be implemented. 
     */
    inline std::string SupportedCapabilitiesResponse() {
        return "\033]+0;0\007";
    }


} // namespace tpp