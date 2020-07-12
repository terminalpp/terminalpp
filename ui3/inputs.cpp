#include "inputs.h"

namespace ui3 {

    Key const Key::Invalid{0};

#define KEY(NAME, CODE) Key const Key::NAME{CODE};
#include "keys.inc.h"

    Key const Key::Shift{0x00010000};
    Key const Key::Ctrl{0x00020000};
    Key const Key::Alt{0x00040000};
    Key const Key::Win{0x00080000};


} // namespace ui