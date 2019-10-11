#pragma once

#include <iostream>
#include <string>

namespace tpp {



    /** Describes the capabilities of attached terminal. 
     */
    class Capabilities {
    public:
        

    };

    Capabilities GetCapabilities(std::ostream & out = std::cout, std::istream & in = std::cin);




} // namespace tpp