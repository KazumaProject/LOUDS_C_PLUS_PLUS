#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <atomic>
#include <cstdint>

struct PrefixNode
{
    char16_t c{u' '};
    int id{-1};
    bool isWord{false};

    std::unordered_map<char16_t, std::unique_ptr<PrefixNode>> children;

    bool hasChild() const { return !children.empty(); }

    PrefixNode *getChild(char16_t ch)
    {
        auto it = children.find(ch);
        return (it == children.end()) ? nullptr : it->second.get();
    }

    const PrefixNode *getChild(char16_t ch) const
    {
        auto it = children.find(ch);
        return (it == children.end()) ? nullptr : it->second.get();
    }

    void addChild(std::unique_ptr<PrefixNode> node)
    {
        const char16_t ch = node->c;
        if (children.find(ch) == children.end())
        {
            children.emplace(ch, std::move(node));
        }
    }
};

class PrefixTree
{
public:
    PrefixTree();

    void insert(const std::u16string &word);

    PrefixNode *getRoot();
    const PrefixNode *getRoot() const;

private:
    std::unique_ptr<PrefixNode> root;
    std::atomic<int> nextId;
};
