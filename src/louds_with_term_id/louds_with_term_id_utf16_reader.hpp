#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <ostream>
#include <istream>
#include <algorithm>

#include "common/bit_vector_utf16.hpp"
#include "common/succinct_bit_vector_utf16.hpp"

// 読み込み専用 LOUDSWithTermId（UTF-16）
// - commonPrefixSearch は文字列のみ返す
// - termId は getTermId(nodeIndex) で取得
class LOUDSWithTermIdUtf16Reader
{
public:
    LOUDSWithTermIdUtf16Reader(const BitVector &lbs,
                               const BitVector &isLeaf,
                               std::vector<char16_t> labels,
                               std::vector<int32_t> termIdsSave);

    std::vector<std::u16string> commonPrefixSearch(const std::u16string &str) const;

    std::u16string getLetter(int nodeIndex) const;

    int getNodeIndex(const std::u16string &s) const;
    int getNodeId(const std::u16string &s) const;

    // leaf nodeIndex を渡す想定
    int32_t getTermId(int nodeIndex) const;

    static LOUDSWithTermIdUtf16Reader loadFromFile(const std::string &path);

private:
    BitVector LBS_;
    BitVector isLeaf_;
    std::vector<char16_t> labels_;
    std::vector<int32_t> termIdsSave_;

    SuccinctBitVector lbsSucc_;
    SuccinctBitVector leafSucc_;

    int firstChild(int pos) const;
    int traverse(int pos, char16_t c) const;

    int search(int index, const std::u16string &chars, size_t wordOffset) const;

    static void read_u64(std::istream &is, uint64_t &v);
    static void read_u16(std::istream &is, uint16_t &v);
    static void read_i32(std::istream &is, int32_t &v);

    static std::vector<uint64_t> read_u64_vec(std::istream &is);
    static BitVector readBitVector(std::istream &is);
};
