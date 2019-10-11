#pragma once

namespace tpp {

    class OSCSequence {
    public:
        static constexpr int Capabilities = 0;
        static constexpr int NewFile = 1;
        static constexpr int Send = 2;
        static constexpr int Open = 3;
    };

} // namespace tpp
