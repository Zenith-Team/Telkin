#pragma once

#include <telkin/Telkin.h>

namespace tk {

    struct GenericHook {
        tk::DataMagic magic;
        u8 _[tk::cHookSize - sizeof(magic)];
    };

} // namespace tk
