#pragma once

namespace tpp {

    /** Terminalpp Sequence base class. 
     
       - deserialize from buffer
       - serialize to buffer
     */
    class Sequence {
    public:
        enum class Kind {
            Invalid,
            Ack,
            Capabilities,
            Data,

        };

        class Invalid;
        class Ack;
        class Capabilities;
        class Data;

    protected:
        Kind kind_;

    }; // tpp::Sequence

    class Sequence::Invalid {

    };

    class Sequence::Ack {

    };

    class Sequence::Capabilities {

    };

    class Sequence::Data : public Sequence {
    protected:

    }; 

} // namespace tpp