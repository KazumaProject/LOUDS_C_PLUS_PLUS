#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <ostream>
#include <istream>

#include "common/bit_vector_utf16.hpp"

// 保存/生成用 LOUDS（writer）
// - convertListToBitVector で BitVector化
// - saveToFile / loadFromFile でバイナリ永続化
class LOUDS
{
public:
    std::vector<bool> LBSTemp;
    std::vector<bool> isLeafTemp;

    BitVector LBS;
    BitVector isLeaf;

    // Kotlin Char 相当（UTF-16 code unit）
    std::vector<char16_t> labels;

    LOUDS();

    void convertListToBitVector();

    std::vector<std::u16string> commonPrefixSearch(const std::u16string &str) const;

    void saveToFile(const std::string &path) const;
    static LOUDS loadFromFile(const std::string &path);

    bool equals(const LOUDS &other) const;

private:
    int firstChild(int pos) const;
    int traverse(int pos, char16_t c) const;

    static void write_u64(std::ostream &os, uint64_t v);
    static void write_u16(std::ostream &os, uint16_t v);

    static void read_u64(std::istream &is, uint64_t &v);
    static void read_u16(std::istream &is, uint16_t &v);

    static void write_u64_vec(std::ostream &os, const std::vector<uint64_t> &v);
    static std::vector<uint64_t> read_u64_vec(std::istream &is);

    void writeBitVector(std::ostream &os, const BitVector &bv) const;
    static BitVector readBitVector(std::istream &is);
};
