#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>

#include "prefix_with_term_id/prefix_tree_with_term_id_utf16.hpp"
#include "louds_with_term_id/converter_with_term_id_utf16.hpp"
#include "louds_with_term_id/louds_with_term_id_utf16_writer.hpp"

static void assert_true(bool cond, const char *msg)
{
    if (!cond)
    {
        std::cerr << "[FAIL] " << msg << "\n";
        std::exit(1);
    }
}

static bool u16_equals(const std::vector<std::u16string> &a,
                       const std::vector<std::u16string> &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

int main()
{
    // termId 付き: writer の検索 + バイナリ round-trip + termId 整合
    {
        PrefixTreeWithTermIdUtf16 t;
        t.insert(u"a");   // termId = 1
        t.insert(u"ab");  // termId = 2
        t.insert(u"abc"); // termId = 3

        ConverterWithTermIdUtf16 conv;
        LOUDSWithTermIdUtf16 louds = conv.convert(t.getRoot());

        auto r = louds.commonPrefixSearch(u"abcd");
        std::vector<std::u16string> expected = {u"a", u"ab", u"abc"};
        assert_true(u16_equals(r, expected), "commonPrefixSearch(u\"abcd\") should be {a,ab,abc}");

        const int idx_a = louds.getNodeIndex(u"a");
        const int idx_ab = louds.getNodeIndex(u"ab");
        const int idx_abc = louds.getNodeIndex(u"abc");
        assert_true(idx_a >= 0, "getNodeIndex(u\"a\") should exist");
        assert_true(idx_ab >= 0, "getNodeIndex(u\"ab\") should exist");
        assert_true(idx_abc >= 0, "getNodeIndex(u\"abc\") should exist");

        assert_true(louds.getTermId(idx_a) == 1, "termId of 'a' should be 1");
        assert_true(louds.getTermId(idx_ab) == 2, "termId of 'ab' should be 2");
        assert_true(louds.getTermId(idx_abc) == 3, "termId of 'abc' should be 3");

        const std::string path = "louds_with_term_id_utf16_writer.bin";
        louds.saveToFile(path);

        LOUDSWithTermIdUtf16 loaded = LOUDSWithTermIdUtf16::loadFromFile(path);
        assert_true(loaded.equals(louds), "binary round-trip should preserve content");

        assert_true(loaded.getTermId(loaded.getNodeIndex(u"a")) == 1, "loaded termId 'a' should be 1");
        assert_true(loaded.getTermId(loaded.getNodeIndex(u"ab")) == 2, "loaded termId 'ab' should be 2");
        assert_true(loaded.getTermId(loaded.getNodeIndex(u"abc")) == 3, "loaded termId 'abc' should be 3");
    }

    // ひらがな（UTF-16）: termId と prefix
    {
        PrefixTreeWithTermIdUtf16 t;
        t.insert(u"す");     // 1
        t.insert(u"すみ");   // 2
        t.insert(u"すみれ"); // 3

        ConverterWithTermIdUtf16 conv;
        LOUDSWithTermIdUtf16 louds = conv.convert(t.getRoot());

        auto r = louds.commonPrefixSearch(u"すみれいろ");
        std::vector<std::u16string> expected = {u"す", u"すみ", u"すみれ"};
        assert_true(u16_equals(r, expected), "commonPrefixSearch(u\"すみれいろ\") should be {す,すみ,すみれ}");

        const int idx = louds.getNodeIndex(u"すみれ");
        assert_true(idx >= 0, "getNodeIndex(u\"すみれ\") should exist");
        assert_true(louds.getTermId(idx) == 3, "termId of 'すみれ' should be 3");
    }

    std::cout << "[OK] LOUDSWithTermId UTF-16 writer tests passed\n";
    return 0;
}
