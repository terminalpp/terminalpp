#pragma once

namespace vterm {

    /** Returns the version of the t++ protocol supported. 
     */
    constexpr int TPP_CAPABILITIES = 0;

    /** Creates new file on the on the target. 
     */
    constexpr int TPP_CREATE_FILE = 1;

    constexpr int TPP_SEND_DATA = 2;

    constexpr int TPP_OPEN_FILE = 3;

} // namespace vterm