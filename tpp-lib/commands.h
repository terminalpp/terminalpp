#pragma once

#include <iostream>
#include <string>

#include "sequence.h"
#include "encoder.h"

/* TODO eventually I want to have an abstraction over the input/output stuff here which for now is assumed to be stdin/stdout. 
 */

namespace tpp {




#if (defined ARCH_LINUX)

    response::Capabilities GetCapabilities(size_t timeout = 100);

    int NewFile(std::string const & path, size_t size, size_t timeout = 100);

    void Send(int fileId, char const * data, size_t numBytes, Encoder & encoder);

    inline void Send(int fileId, char const * data, size_t numBytes) {
        Encoder enc;
        Send(fileId, data, numBytes, enc);
    }

    void OpenFile(int fileId);
    
#endif




} // namespace tpp