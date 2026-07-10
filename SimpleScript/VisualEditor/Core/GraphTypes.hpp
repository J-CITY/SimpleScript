#pragma once
#include <cstdint>

namespace Visual {

    using Id = int;

    struct Vec2 {
        float x = 0.f, y = 0.f;
        Vec2() = default;
        Vec2(float x, float y) : x(x), y(y) {}
    };

    struct ColorRGBA {
        float r = 1.f, g = 1.f, b = 1.f, a = 1.f;
        ColorRGBA() = default;
        ColorRGBA(float r, float g, float b, float a = 1.f) : r(r), g(g), b(b), a(a) {}
        // Convenience: integer 0-255 channel values
        ColorRGBA(int r, int g, int b, int a = 255)
            : r(r / 255.f), g(g / 255.f), b(b / 255.f), a(a / 255.f) {}
    };

} // namespace Visual
