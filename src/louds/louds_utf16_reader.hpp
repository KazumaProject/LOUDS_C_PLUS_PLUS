#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <algorithm>

#include "common/bit_vector_utf16.hpp"
#include "common/succinct_bit_vector_utf16.hpp"

// 読み込み専用 LOUDSReader
// - loadFromFile でロード
// - 内部で SuccinctBitVector を構築し rank/select を高速化
class LOUDSReader
{
public:
    LOUDSReader(const BitVector &lbs,
                const BitVector &isLeaf,
                std::vector<char16_t> labels);

    std::vector<std::u16string> commonPrefixSearch(const std::u16string &str) const;

    // ルートから nodeIndex までのラベルを復元
    std::u16string getLetter(int nodeIndex) const;

    int getNodeIndex(const std::u16string &s) const;
    int getNodeId(const std::u16string &s) const;

    const std::vector<char16_t> &getAllLabels() const { return labels_; }

    static LOUDSReader loadFromFile(const std::string &path);

private:
    BitVector LBS_;
    BitVector isLeaf_;
    std::vector<char16_t> labels_;

    SuccinctBitVector lbsSucc_;

    int firstChild(int pos) const;
    int traverse(int pos, char16_t c) const;

    int search(int index, const std::u16string &chars, size_t wordOffset) const;

    static void read_u64(std::istream &is, uint64_t &v);
    static void read_u16(std::istream &is, uint16_t &v);
    static std::vector<uint64_t> read_u64_vec(std::istream &is);
    static BitVector readBitVector(std::istream &is);
};
