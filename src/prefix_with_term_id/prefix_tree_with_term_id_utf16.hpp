#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <atomic>
#include <cstdint>

struct PrefixNodeWithTermIdUtf16
{
    char16_t c{u' '};
    int id{-1};
    bool isWord{false};
    int32_t termId{-1};

    std::unordered_map<char16_t, std::unique_ptr<PrefixNodeWithTermIdUtf16>> children;

    bool hasChild() const { return !children.empty(); }

    PrefixNodeWithTermIdUtf16 *getChild(char16_t ch)
    {
        auto it = children.find(ch);
        return (it == children.end()) ? nullptr : it->second.get();
    }

    const PrefixNodeWithTermIdUtf16 *getChild(char16_t ch) const
    {
        auto it = children.find(ch);
        return (it == children.end()) ? nullptr : it->second.get();
    }

    void addChild(std::unique_ptr<PrefixNodeWithTermIdUtf16> node)
    {
        const char16_t ch = node->c;
        if (children.find(ch) == children.end())
        {
            children.emplace(ch, std::move(node));
        }
    }
};

class PrefixTreeWithTermIdUtf16
{
public:
    PrefixTreeWithTermIdUtf16();

    void insert(const std::u16string &word);

    PrefixNodeWithTermIdUtf16 *getRoot();
    const PrefixNodeWithTermIdUtf16 *getRoot() const;

    int getNodeSize() const;
    int getTermIdSize() const;

private:
    std::unique_ptr<PrefixNodeWithTermIdUtf16> root;
    std::atomic<int> nextNodeId;
    std::atomic<int32_t> nextTermId;
};
