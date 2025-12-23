#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>

#include "prefix_with_term_id/prefix_tree_with_term_id_utf16.hpp"
#include "louds_with_term_id/converter_with_term_id_utf16.hpp"
#include "louds_with_term_id/louds_with_term_id_utf16_writer.hpp"
#include "louds_with_term_id/louds_with_term_id_utf16_reader.hpp"

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
    // writer->save, reader->load, prefix と termId 整合
    {
        PrefixTreeWithTermIdUtf16 t;
        t.insert(u"a");   // 1
        t.insert(u"ab");  // 2
        t.insert(u"abc"); // 3

        ConverterWithTermIdUtf16 conv;
        LOUDSWithTermIdUtf16 louds = conv.convert(t.getRoot());

        const std::string path = "louds_with_term_id_utf16_reader.bin";
        louds.saveToFile(path);

        LOUDSWithTermIdUtf16Reader reader = LOUDSWithTermIdUtf16Reader::loadFromFile(path);

        auto r = reader.commonPrefixSearch(u"abcd");
        std::vector<std::u16string> expected = {u"a", u"ab", u"abc"};
        assert_true(u16_equals(r, expected), "reader commonPrefixSearch should be {a,ab,abc}");

        const int idx_a = reader.getNodeIndex(u"a");
        const int idx_ab = reader.getNodeIndex(u"ab");
        const int idx_abc = reader.getNodeIndex(u"abc");
        assert_true(idx_a >= 0, "reader getNodeIndex(u\"a\") should exist");
        assert_true(idx_ab >= 0, "reader getNodeIndex(u\"ab\") should exist");
        assert_true(idx_abc >= 0, "reader getNodeIndex(u\"abc\") should exist");

        assert_true(reader.getLetter(idx_abc) == u"abc", "reader getLetter(nodeIndex_of_abc) should be u\"abc\"");

        assert_true(reader.getTermId(idx_a) == 1, "reader termId of 'a' should be 1");
        assert_true(reader.getTermId(idx_ab) == 2, "reader termId of 'ab' should be 2");
        assert_true(reader.getTermId(idx_abc) == 3, "reader termId of 'abc' should be 3");
    }

    std::cout << "[OK] LOUDSWithTermId UTF-16 reader tests passed\n";
    return 0;
}
