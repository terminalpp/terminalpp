#include "helpers/helpers.h"

#include "sequences.h"
#include "commands.h"

namespace tpp {

    namespace {

        constexpr char const * TPP_ESCAPE = "\033]+";
        constexpr char const * TPP_END = "\007";

        std::string Read(std::istream & in) {

            NOT_IMPLEMENTED;
        }
    }

    Capabilities GetCapabilities(std::ostream & out, std::istream & in) {
        {
            std::string x(STR(TPP_ESCAPE << OSCSequence::Capabilities << TPP_END));
            out << x << std::flush;
        }
        NOT_IMPLEMENTED;
    }



}