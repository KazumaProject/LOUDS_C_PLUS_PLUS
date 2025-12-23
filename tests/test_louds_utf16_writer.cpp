#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>

#include "prefix/prefix_tree_utf16.hpp"
#include "louds/louds_converter_utf16.hpp"
#include "louds/louds_utf16_writer.hpp"

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
    // 1) ASCII commonPrefixSearch + バイナリround-trip
    {
        PrefixTreeUtf16 t;
        t.insert(u"a");
        t.insert(u"ab");
        t.insert(u"abc");

        ConverterUtf16 conv;
        LOUDSUtf16 louds = conv.convert(t.getRoot());

        auto r = louds.commonPrefixSearch(u"abcd");
        std::vector<std::u16string> expected = {u"a", u"ab", u"abc"};
        assert_true(u16_equals(r, expected),
                    "commonPrefixSearch(u\"abcd\") should be {a,ab,abc}");

        assert_true(louds.LBS.size() > 2, "LBS should be non-trivial");
        assert_true(louds.labels.size() >= 2, "labels should have init elements");
        assert_true(louds.isLeaf.size() == louds.LBS.size(), "isLeaf size should match LBS size");

        const std::string path = "louds_test_utf16_writer.bin";
        louds.saveToFile(path);

        LOUDSUtf16 loaded = LOUDSUtf16::loadFromFile(path);
        assert_true(loaded.equals(louds),
                    "LOUDS binary round-trip should preserve content");

        auto r1 = louds.commonPrefixSearch(u"abcd");
        auto r2 = loaded.commonPrefixSearch(u"abcd");
        assert_true(u16_equals(r1, r2),
                    "commonPrefixSearch should match after load");
    }

    // 2) ひらがな commonPrefixSearch
    {
        PrefixTreeUtf16 ht;
        ht.insert(u"か");
        ht.insert(u"かな");
        ht.insert(u"かなえ");
        ht.insert(u"かなる");

        ConverterUtf16 hconv;
        LOUDSUtf16 hlouds = hconv.convert(ht.getRoot());

        auto r = hlouds.commonPrefixSearch(u"かなえた");
        std::vector<std::u16string> expected = {u"か", u"かな", u"かなえ"};
        assert_true(u16_equals(r, expected),
                    "hiragana commonPrefixSearch(u\"かなえた\") should be {か,かな,かなえ}");

        assert_true(hlouds.LBS.size() > 2, "hiragana: LBS should be non-trivial");
        assert_true(hlouds.labels.size() >= 2, "hiragana: labels should have init elements");
        assert_true(hlouds.isLeaf.size() == hlouds.LBS.size(), "hiragana: isLeaf size should match LBS size");
    }

    // 3) ひらがな binary round-trip
    {
        PrefixTreeUtf16 ht;
        ht.insert(u"す");
        ht.insert(u"すみ");
        ht.insert(u"すみれ");

        ConverterUtf16 hconv;
        LOUDSUtf16 hlouds = hconv.convert(ht.getRoot());

        const std::string path = "louds_hiragana_test_utf16_writer.bin";
        hlouds.saveToFile(path);

        LOUDSUtf16 loaded = LOUDSUtf16::loadFromFile(path);

        assert_true(loaded.equals(hlouds),
                    "hiragana LOUDS binary round-trip should preserve content");

        auto r1 = hlouds.commonPrefixSearch(u"すみれいろ");
        auto r2 = loaded.commonPrefixSearch(u"すみれいろ");
        std::vector<std::u16string> expected = {u"す", u"すみ", u"すみれ"};

        assert_true(u16_equals(r1, expected),
                    "hiragana commonPrefixSearch before save should be {す,すみ,すみれ}");
        assert_true(u16_equals(r2, expected),
                    "hiragana commonPrefixSearch after load should be {す,すみ,すみれ}");
    }

    std::cout << "[OK] all LOUDS UTF-16 writer tests passed\n";
    return 0;
}
