#pragma once

constexpr int ascii_char_len = 70;
constexpr char ascii_char[] = " .'`^\",:;Il!i><~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";

inline static char color2ascii(int& r, int& g, int& b) { // Luminosity method: https://en.wikipedia.org/wiki/Relative_luminance
    const int index = (int)((0.2126f * r + 0.7152f * g + 0.0722f * b) / 3.0f * ascii_char_len / 255.0f);
    return ascii_char[index];
}
