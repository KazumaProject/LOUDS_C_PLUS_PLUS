#pragma once
#include "prefix/prefix_tree_utf16.hpp"
#include "louds/louds_utf16_writer.hpp"

class Converter
{
public:
    LOUDS convert(const PrefixNode *rootNode) const;
};
