#pragma once
#include "VisualTypes.h"

namespace Visual
{
    struct IdGenerator {
        inline static Id GenId() {
            return id++;
        }

        inline static void SetStart(Id _id) {
            id = _id;
        }
    private:
        inline static int id = 1;
    };

}


