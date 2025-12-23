#include "louds_with_term_id/converter_with_term_id_utf16.hpp"
#include <queue>

LOUDSWithTermIdUtf16 ConverterWithTermIdUtf16::convert(const PrefixNodeWithTermIdUtf16 *rootNode) const
{
    LOUDSWithTermIdUtf16 louds;

    std::queue<const PrefixNodeWithTermIdUtf16 *> q;
    q.push(rootNode);

    while (!q.empty())
    {
        const PrefixNodeWithTermIdUtf16 *node = q.front();
        q.pop();

        if (node && node->hasChild())
        {
            for (const auto &kv : node->children)
            {
                const char16_t label = kv.first;
                const PrefixNodeWithTermIdUtf16 *child = kv.second.get();

                q.push(child);

                louds.LBSTemp.push_back(true);
                louds.labels.push_back(label);
                louds.isLeafTemp.push_back(child->isWord);

                // leaf の出現順で termId を保存
                if (child->isWord)
                {
                    louds.termIdsSave.push_back(static_cast<int32_t>(child->termId));
                }
            }
        }

        louds.LBSTemp.push_back(false);
        louds.isLeafTemp.push_back(false);
    }

    louds.convertListToBitVector();
    return louds;
}
