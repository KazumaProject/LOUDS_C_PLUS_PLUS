#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <ostream>
#include <istream>
#include <stdexcept>

#include "common/bit_vector_utf16.hpp"

// 保存/生成用 LOUDSWithTermId（UTF-16 / char16_t）
class LOUDSWithTermIdUtf16
{
public:
    // builder 用（BFS で push して最後に BitVector 化）
    std::vector<bool> LBSTemp;
    std::vector<bool> isLeafTemp;

    // LOUDS 本体
    BitVector LBS;
    BitVector isLeaf;
    std::vector<char16_t> labels;

    // leaf ノードに対応する termId 配列（leaf の出現順）
    std::vector<int32_t> termIdsSave;

    LOUDSWithTermIdUtf16();

    void convertListToBitVector();

    // termId を同時に返さない（要件どおり）
    std::vector<std::u16string> commonPrefixSearch(const std::u16string &str) const;

    // leaf nodeIndex に対して使う想定
    int32_t getTermId(int nodeIndex) const;

    int getNodeIndex(const std::u16string &s) const;
    int getNodeId(const std::u16string &s) const;

    void saveToFile(const std::string &path) const;
    static LOUDSWithTermIdUtf16 loadFromFile(const std::string &path);

    bool equals(const LOUDSWithTermIdUtf16 &other) const;

private:
    int firstChild(int pos) const;
    int traverse(int pos, char16_t c) const;

    // Kotlin: search / indexOfLabel 相当
    int search(int index, const std::u16string &chars, size_t wordOffset) const;
    int indexOfLabel(int label) const;

    static void write_u64(std::ostream &os, uint64_t v);
    static void write_u16(std::ostream &os, uint16_t v);
    static void write_i32(std::ostream &os, int32_t v);

    static void read_u64(std::istream &is, uint64_t &v);
    static void read_u16(std::istream &is, uint16_t &v);
    static void read_i32(std::istream &is, int32_t &v);

    static void write_u64_vec(std::ostream &os, const std::vector<uint64_t> &v);
    static std::vector<uint64_t> read_u64_vec(std::istream &is);

    void writeBitVector(std::ostream &os, const BitVector &bv) const;
    static BitVector readBitVector(std::istream &is);
};
