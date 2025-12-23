#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>

#include "prefix/prefix_tree_utf16.hpp"
#include "louds/louds_converter_utf16.hpp"
#include "louds/louds_utf16_writer.hpp"
#include "louds/louds_utf16_reader.hpp"

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
    // 1) ASCII: writer->save, reader->load, commonPrefixSearch一致
    {
        PrefixTreeUtf16 t;
        t.insert(u"a");
        t.insert(u"ab");
        t.insert(u"abc");

        ConverterUtf16 conv;
        LOUDSUtf16 louds = conv.convert(t.getRoot());

        const std::string path = "louds_writer_ascii_utf16.bin";
        louds.saveToFile(path);

        LOUDSReaderUtf16 reader = LOUDSReaderUtf16::loadFromFile(path);

        auto r = reader.commonPrefixSearch(u"abcd");
        std::vector<std::u16string> expected = {u"a", u"ab", u"abc"};
        assert_true(u16_equals(r, expected),
                    "reader ASCII commonPrefixSearch should be {a,ab,abc}");

        const int idx = reader.getNodeIndex(u"abc");
        assert_true(idx >= 0, "reader getNodeIndex(u\"abc\") should exist");

        const std::u16string s = reader.getLetter(idx);
        assert_true(s == u"abc", "reader getLetter(nodeIndex_of_abc) should be u\"abc\"");
    }

    // 2) ひらがな: writer->save, reader->load, commonPrefixSearch一致
    {
        PrefixTreeUtf16 t;
        t.insert(u"す");
        t.insert(u"すみ");
        t.insert(u"すみれ");

        ConverterUtf16 conv;
        LOUDSUtf16 louds = conv.convert(t.getRoot());

        const std::string path = "louds_writer_hira_utf16.bin";
        louds.saveToFile(path);

        LOUDSReaderUtf16 reader = LOUDSReaderUtf16::loadFromFile(path);

        auto r = reader.commonPrefixSearch(u"すみれいろ");
        std::vector<std::u16string> expected = {u"す", u"すみ", u"すみれ"};
        assert_true(u16_equals(r, expected),
                    "reader hiragana commonPrefixSearch should be {す,すみ,すみれ}");

        const int idx = reader.getNodeIndex(u"すみれ");
        assert_true(idx >= 0, "reader getNodeIndex(u\"すみれ\") should exist");

        const std::u16string s = reader.getLetter(idx);
        assert_true(s == u"すみれ", "reader getLetter(nodeIndex_of_すみれ) should be u\"すみれ\"");
    }

    std::cout << "[OK] LOUDS UTF-16 reader tests passed\n";
    return 0;
}
