#include "prefix_with_term_id/prefix_tree_with_term_id_utf16.hpp"

PrefixTreeWithTermIdUtf16::PrefixTreeWithTermIdUtf16()
    : root(std::make_unique<PrefixNodeWithTermIdUtf16>()),
      nextNodeId(1),
      nextTermId(1)
{
}

void PrefixTreeWithTermIdUtf16::insert(const std::u16string &word)
{
    PrefixNodeWithTermIdUtf16 *cur = root.get();

    const int32_t termId = nextTermId.fetch_add(1);

    for (char16_t ch : word)
    {
        if (auto *nxt = cur->getChild(ch))
        {
            cur = nxt;
        }
        else
        {
            // 既存実装の雰囲気を踏襲: id は 2 から始まる
            int id = nextNodeId.fetch_add(1) + 1;

            auto node = std::make_unique<PrefixNodeWithTermIdUtf16>();
            node->c = ch;
            node->id = id;
            node->termId = termId;

            PrefixNodeWithTermIdUtf16 *raw = node.get();
            cur->addChild(std::move(node));
            cur = raw;
        }
    }

    cur->isWord = true;
}

PrefixNodeWithTermIdUtf16 *PrefixTreeWithTermIdUtf16::getRoot() { return root.get(); }
const PrefixNodeWithTermIdUtf16 *PrefixTreeWithTermIdUtf16::getRoot() const { return root.get(); }

int PrefixTreeWithTermIdUtf16::getNodeSize() const
{
    return nextNodeId.load();
}

int PrefixTreeWithTermIdUtf16::getTermIdSize() const
{
    return static_cast<int>(nextTermId.load());
}
