#include "louds_with_term_id/louds_with_term_id_utf16_reader.hpp"

LOUDSWithTermIdUtf16Reader::LOUDSWithTermIdUtf16Reader(const BitVector &lbs,
                                                       const BitVector &isLeaf,
                                                       std::vector<char16_t> labels,
                                                       std::vector<int32_t> termIdsSave)
    : LBS_(lbs),
      isLeaf_(isLeaf),
      labels_(std::move(labels)),
      termIdsSave_(std::move(termIdsSave)),
      lbsSucc_(LBS_),
      leafSucc_(isLeaf_)
{
}

int LOUDSWithTermIdUtf16Reader::firstChild(int pos) const
{
    const int y = lbsSucc_.select0(lbsSucc_.rank1(pos)) + 1;
    if (y < 0)
        return -1;
    if (static_cast<size_t>(y) >= LBS_.size())
        return -1;
    return (LBS_.get(static_cast<size_t>(y)) ? y : -1);
}

int LOUDSWithTermIdUtf16Reader::traverse(int pos, char16_t c) const
{
    int childPos = firstChild(pos);
    if (childPos == -1)
        return -1;

    while (static_cast<size_t>(childPos) < LBS_.size() &&
           LBS_.get(static_cast<size_t>(childPos)))
    {
        const int labelIndex = lbsSucc_.rank1(childPos);
        if (labelIndex >= 0 && static_cast<size_t>(labelIndex) < labels_.size())
        {
            if (labels_[static_cast<size_t>(labelIndex)] == c)
                return childPos;
        }
        childPos += 1;
    }
    return -1;
}

std::vector<std::u16string> LOUDSWithTermIdUtf16Reader::commonPrefixSearch(const std::u16string &str) const
{
    std::vector<char16_t> resultTemp;
    std::vector<std::u16string> result;

    int n = 0;
    for (char16_t c : str)
    {
        n = traverse(n, c);
        if (n == -1)
            break;

        const int index = lbsSucc_.rank1(n);
        if (index < 0 || static_cast<size_t>(index) >= labels_.size())
            break;

        resultTemp.push_back(labels_[static_cast<size_t>(index)]);

        if (static_cast<size_t>(n) < isLeaf_.size() && isLeaf_.get(static_cast<size_t>(n)))
        {
            std::u16string tempStr(resultTemp.begin(), resultTemp.end());
            if (!result.empty())
            {
                result.push_back(result[0] + tempStr);
            }
            else
            {
                result.push_back(tempStr);
                resultTemp.clear();
            }
        }
    }
    return result;
}

std::u16string LOUDSWithTermIdUtf16Reader::getLetter(int nodeIndex) const
{
    if (nodeIndex < 0)
        return u"";
    if (static_cast<size_t>(nodeIndex) >= LBS_.size())
        return u"";

    std::u16string out;
    int current = nodeIndex;

    while (true)
    {
        const int nodeId = lbsSucc_.rank1(current);
        if (nodeId < 0 || static_cast<size_t>(nodeId) >= labels_.size())
            break;

        const char16_t ch = labels_[static_cast<size_t>(nodeId)];
        if (ch != u' ')
            out.push_back(ch);

        if (nodeId == 0)
            break;

        const int r0 = lbsSucc_.rank0(current);
        current = lbsSucc_.select1(r0);
        if (current < 0)
            break;
    }

    std::reverse(out.begin(), out.end());
    return out;
}

int LOUDSWithTermIdUtf16Reader::getNodeIndex(const std::u16string &s) const
{
    return search(2, s, 0);
}

int LOUDSWithTermIdUtf16Reader::getNodeId(const std::u16string &s) const
{
    const int idx = getNodeIndex(s);
    if (idx < 0)
        return -1;
    return lbsSucc_.rank0(idx);
}

int32_t LOUDSWithTermIdUtf16Reader::getTermId(int nodeIndex) const
{
    if (nodeIndex < 0)
        return -1;
    if (static_cast<size_t>(nodeIndex) >= isLeaf_.size())
        return -1;

    if (!isLeaf_.get(static_cast<size_t>(nodeIndex)))
        return -1;

    // leaf の rank1(nodeIndex) - 1 を termIdsSave_ の index にする
    const int leafRank = leafSucc_.rank1(nodeIndex);
    const int leafIndex = leafRank - 1;
    if (leafIndex < 0)
        return -1;
    if (static_cast<size_t>(leafIndex) >= termIdsSave_.size())
        return -1;

    return termIdsSave_[static_cast<size_t>(leafIndex)];
}

int LOUDSWithTermIdUtf16Reader::search(int index, const std::u16string &chars, size_t wordOffset) const
{
    int currentIndex = index;
    if (chars.empty())
        return -1;
    if (currentIndex < 0)
        return -1;

    while (static_cast<size_t>(currentIndex) < LBS_.size() && LBS_.get(static_cast<size_t>(currentIndex)))
    {
        if (wordOffset >= chars.size())
            return currentIndex;

        int charIndex = lbsSucc_.rank1(currentIndex);
        if (charIndex < 0 || static_cast<size_t>(charIndex) >= labels_.size())
            return -1;

        const char16_t currentLabel = labels_[static_cast<size_t>(charIndex)];
        const char16_t currentChar = chars[wordOffset];

        if (currentChar == currentLabel)
        {
            if (wordOffset + 1 == chars.size())
            {
                return currentIndex;
            }
            const int nextIndex = lbsSucc_.select0(charIndex) + 1;
            if (nextIndex < 0)
                return -1;
            return search(nextIndex, chars, wordOffset + 1);
        }

        currentIndex++;
    }
    return -1;
}

void LOUDSWithTermIdUtf16Reader::read_u64(std::istream &is, uint64_t &v)
{
    is.read(reinterpret_cast<char *>(&v), sizeof(v));
}
void LOUDSWithTermIdUtf16Reader::read_u16(std::istream &is, uint16_t &v)
{
    is.read(reinterpret_cast<char *>(&v), sizeof(v));
}
void LOUDSWithTermIdUtf16Reader::read_i32(std::istream &is, int32_t &v)
{
    is.read(reinterpret_cast<char *>(&v), sizeof(v));
}

std::vector<uint64_t> LOUDSWithTermIdUtf16Reader::read_u64_vec(std::istream &is)
{
    uint64_t n = 0;
    read_u64(is, n);
    std::vector<uint64_t> v(static_cast<size_t>(n));
    if (n > 0)
    {
        is.read(reinterpret_cast<char *>(v.data()),
                static_cast<std::streamsize>(n * sizeof(uint64_t)));
    }
    return v;
}

BitVector LOUDSWithTermIdUtf16Reader::readBitVector(std::istream &is)
{
    uint64_t nbits = 0;
    read_u64(is, nbits);
    auto words = read_u64_vec(is);
    BitVector bv;
    bv.assign_from_words(static_cast<size_t>(nbits), std::move(words));
    return bv;
}

LOUDSWithTermIdUtf16Reader LOUDSWithTermIdUtf16Reader::loadFromFile(const std::string &path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("failed to open file for read: " + path);

    BitVector lbs = readBitVector(ifs);
    BitVector isLeaf = readBitVector(ifs);

    // labels
    uint64_t labelN = 0;
    read_u64(ifs, labelN);

    std::vector<char16_t> labels;
    labels.resize(static_cast<size_t>(labelN));
    for (size_t i = 0; i < static_cast<size_t>(labelN); ++i)
    {
        uint16_t v = 0;
        read_u16(ifs, v);
        labels[i] = static_cast<char16_t>(v);
    }

    // termIdsSave
    uint64_t termN = 0;
    read_u64(ifs, termN);

    std::vector<int32_t> termIds;
    termIds.resize(static_cast<size_t>(termN));
    for (size_t i = 0; i < static_cast<size_t>(termN); ++i)
    {
        int32_t v = 0;
        read_i32(ifs, v);
        termIds[i] = v;
    }

    return LOUDSWithTermIdUtf16Reader(lbs, isLeaf, std::move(labels), std::move(termIds));
}
