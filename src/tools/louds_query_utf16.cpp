// src/tools/louds_query_utf16.cpp
//
// Usage:
//   louds_query_utf16 <dict.louds_utf16.bin> <query_utf8>
//
// Example:
//   ./louds_query_utf16 ../out/jawiki_latest_utf16.louds_utf16.bin "東京"
//
// Notes:
// - LOUDS UTF-16 expects std::u16string internally.
// - This tool converts UTF-8 -> UTF-16 for querying.
// - For printing results, it converts UTF-16 -> UTF-8.
// - commonPrefixSearch() returns prefixes (subject to your LOUDS implementation behavior).

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include "louds/louds_utf16_writer.hpp"

// -----------------------------
// UTF-8 -> UTF-16 (strict, minimal, no ICU)
// -----------------------------
static bool utf8_to_u16(const std::string &s, std::u16string &out)
{
    out.clear();
    out.reserve(s.size());

    const unsigned char *p = reinterpret_cast<const unsigned char *>(s.data());
    size_t i = 0;
    const size_t n = s.size();

    while (i < n)
    {
        unsigned char c = p[i];

        if (c <= 0x7F)
        {
            out.push_back(static_cast<char16_t>(c));
            i += 1;
            continue;
        }

        int len = 0;
        char32_t cp = 0;

        if ((c & 0xE0) == 0xC0)
        {
            len = 2;
            cp = static_cast<char32_t>(c & 0x1F);
        }
        else if ((c & 0xF0) == 0xE0)
        {
            len = 3;
            cp = static_cast<char32_t>(c & 0x0F);
        }
        else if ((c & 0xF8) == 0xF0)
        {
            len = 4;
            cp = static_cast<char32_t>(c & 0x07);
        }
        else
        {
            return false;
        }

        if (i + static_cast<size_t>(len) > n)
            return false;

        for (int k = 1; k < len; ++k)
        {
            unsigned char cc = p[i + static_cast<size_t>(k)];
            if ((cc & 0xC0) != 0x80)
                return false;
            cp = (cp << 6) | static_cast<char32_t>(cc & 0x3F);
        }

        // Reject overlong encodings
        if (len == 2 && cp < 0x80)
            return false;
        if (len == 3 && cp < 0x800)
            return false;
        if (len == 4 && cp < 0x10000)
            return false;

        // Surrogates invalid in UTF-8 scalar values
        if (cp >= 0xD800 && cp <= 0xDFFF)
            return false;

        if (cp > 0x10FFFF)
            return false;

        if (cp <= 0xFFFF)
        {
            out.push_back(static_cast<char16_t>(cp));
        }
        else
        {
            cp -= 0x10000;
            char16_t hi = static_cast<char16_t>(0xD800 + ((cp >> 10) & 0x3FF));
            char16_t lo = static_cast<char16_t>(0xDC00 + (cp & 0x3FF));
            out.push_back(hi);
            out.push_back(lo);
        }

        i += static_cast<size_t>(len);
    }

    return true;
}

// -----------------------------
// UTF-16 -> UTF-8 (minimal, no ICU)
// - validates surrogate pairs
// - replaces invalid sequences with '?'
// -----------------------------
static std::string u16_to_utf8(const std::u16string &s)
{
    std::string out;
    out.reserve(s.size() * 3);

    auto emit_utf8 = [&](char32_t cp)
    {
        if (cp <= 0x7F)
        {
            out.push_back(static_cast<char>(cp));
        }
        else if (cp <= 0x7FF)
        {
            out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else if (cp <= 0xFFFF)
        {
            // skip surrogate range
            if (cp >= 0xD800 && cp <= 0xDFFF)
            {
                out.push_back('?');
                return;
            }
            out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else if (cp <= 0x10FFFF)
        {
            out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else
        {
            out.push_back('?');
        }
    };

    for (size_t i = 0; i < s.size(); ++i)
    {
        char16_t cu = s[i];

        // high surrogate
        if (cu >= 0xD800 && cu <= 0xDBFF)
        {
            if (i + 1 >= s.size())
            {
                out.push_back('?');
                continue;
            }
            char16_t cu2 = s[i + 1];
            if (cu2 < 0xDC00 || cu2 > 0xDFFF)
            {
                out.push_back('?');
                continue;
            }

            char32_t hi = static_cast<char32_t>(cu - 0xD800);
            char32_t lo = static_cast<char32_t>(cu2 - 0xDC00);
            char32_t cp = 0x10000 + ((hi << 10) | lo);

            emit_utf8(cp);
            i += 1;
            continue;
        }

        // stray low surrogate
        if (cu >= 0xDC00 && cu <= 0xDFFF)
        {
            out.push_back('?');
            continue;
        }

        emit_utf8(static_cast<char32_t>(cu));
    }

    return out;
}

static void usage(const char *prog)
{
    std::cerr
        << "Usage:\n"
        << "  " << prog << " <dict.louds_utf16.bin> <query_utf8>\n"
        << "\n"
        << "Example:\n"
        << "  " << prog << " ../out/jawiki_latest_utf16.louds_utf16.bin \"東京\"\n";
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argv[0]);
        return 2;
    }

    const std::string dict_path = argv[1];
    const std::string query_utf8 = argv[2];

    try
    {
        // 1) load dict (UTF-16)
        LOUDS dict = LOUDS::loadFromFile(dict_path);

        // 2) UTF-8 -> UTF-16
        std::u16string query_u16;
        if (!utf8_to_u16(query_utf8, query_u16))
        {
            std::cerr << "Invalid UTF-8 query.\n";
            return 1;
        }

        // 3) query
        auto res = dict.commonPrefixSearch(query_u16);

        // 4) output
        std::cout << "dict=" << dict_path << "\n";
        std::cout << "query=" << query_utf8 << "\n";
        std::cout << "hit=" << res.size() << "\n";
        for (const auto &u16 : res)
        {
            std::cout << u16_to_utf8(u16) << "\n";
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
}
