#pragma once

#include <string>

namespace tpp {

    class OSCSequence {
    public:
        static constexpr int Capabilities = 0;
        static constexpr int CreateFile = 1;
        static constexpr int Send = 2;
        static constexpr int Open = 3;
    };

    /** Returns the t++ capabilities identifier as supported by this implementation. 
     
        For now only returns version 0. In the future this should be parametrized by various capabilities that may or may not be implemented. 
     */
    inline std::string SupportedCapabilitiesResponse() {
        return "\033]+0;0\007";
    }


} // namespace tpp