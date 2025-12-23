#pragma once
#include "prefix/prefix_tree_utf16.hpp"
#include "louds/louds_utf16_writer.hpp"

// UTF-16 版 Converter は char32_t 版 Converter と衝突するため別名。
class ConverterUtf16
{
public:
    LOUDSUtf16 convert(const PrefixNodeUtf16 *rootNode) const;
};
