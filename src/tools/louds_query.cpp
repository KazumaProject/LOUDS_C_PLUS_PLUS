// src/tools/louds_query.cpp
//
// Usage:
//   louds_query <dict.louds.bin> <query_utf8>
//
// Example:
//   ./louds_query ../out/jawiki_latest.louds.bin "東京"
//
// Notes:
// - LOUDS expects UTF-32 internally. This tool converts UTF-8 -> UTF-32 for querying.
// - For printing results, it converts UTF-32 -> UTF-8.
// - commonPrefixSearch() returns prefixes (subject to your LOUDS implementation behavior).

#include <iostream>
#include <string>
#include <vector>

#include "louds/louds.hpp"

// -----------------------------
// UTF-8 -> UTF-32 (char32_t) decoder (strict, minimal, no ICU)
// -----------------------------
static bool utf8_to_u32(const std::string &s, std::u32string &out)
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
            out.push_back(static_cast<char32_t>(c));
            i += 1;
            continue;
        }

        int len = 0;
        char32_t cp = 0;

        if ((c & 0xE0) == 0xC0)
        {
            len = 2;
            cp = c & 0x1F;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            len = 3;
            cp = c & 0x0F;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            len = 4;
            cp = c & 0x07;
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
            cp = (cp << 6) | (cc & 0x3F);
        }

        // Reject overlong encodings and invalid ranges
        if (len == 2 && cp < 0x80)
            return false;
        if (len == 3 && cp < 0x800)
            return false;
        if (len == 4 && cp < 0x10000)
            return false;

        // Surrogates are invalid in UTF-8
        if (cp >= 0xD800 && cp <= 0xDFFF)
            return false;

        // Max Unicode scalar value
        if (cp > 0x10FFFF)
            return false;

        out.push_back(cp);
        i += static_cast<size_t>(len);
    }
    return true;
}

// -----------------------------
// UTF-32 -> UTF-8 encoder (minimal, no ICU)
// -----------------------------
static std::string u32_to_utf8(const std::u32string &s)
{
    std::string out;
    out.reserve(s.size() * 3);

    for (char32_t cp : s)
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
            // Skip surrogate range (should not appear in valid UTF-32 strings)
            if (cp >= 0xD800 && cp <= 0xDFFF)
            {
                out.push_back('?');
                continue;
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
    }
    return out;
}

static void usage(const char *prog)
{
    std::cerr
        << "Usage:\n"
        << "  " << prog << " <dict.louds.bin> <query_utf8>\n"
        << "\n"
        << "Example:\n"
        << "  " << prog << " ../out/jawiki_latest.louds.bin \"東京\"\n";
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
        // 1) load dict
        LOUDS dict = LOUDS::loadFromFile(dict_path);

        // 2) UTF-8 -> UTF-32
        std::u32string query_u32;
        if (!utf8_to_u32(query_utf8, query_u32))
        {
            std::cerr << "Invalid UTF-8 query.\n";
            return 1;
        }

        // 3) query
        auto res = dict.commonPrefixSearch(query_u32);

        // 4) output
        std::cout << "dict=" << dict_path << "\n";
        std::cout << "query=" << query_utf8 << "\n";
        std::cout << "hit=" << res.size() << "\n";
        for (const auto &u32 : res)
        {
            std::cout << u32_to_utf8(u32) << "\n";
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
}
