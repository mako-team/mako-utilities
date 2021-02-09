// -----------------------------------------------------------------------
//  <copyright file="BookMarkTreeNode.h" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations regarding the use or
//  results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#pragma once
#include <jawsmako/jawsmako.h>
#include <edl/idomoutline.h>

#include <vector>
#include <set>
#include <map>

using namespace EDL;
using namespace JawsMako;

typedef std::map<DOMid, int> PageIdToIndexMap;

class BookmarkTreeNode
{
public:
    static BookmarkTreeNode createFromDocument(const IDocumentPtr& document, int startPageIndex, int endPageIndex);

    uint32 getChildCount(bool recurse = false) const;
    //void appendToDocument(const IDocumentPtr& targetDocument, int sourceToTargetPageDelta, const IJawsMakoPtr& mako) const;
    void appendToDocument(const IDocumentPtr& targetDocument, int sourceToTargetPageDelta, const IJawsMakoPtr& mako, const IDOMOutlineTreeNodePtr& targetOutline) const;

private:
    BookmarkTreeNode(const IDocumentPtr& document) : BookmarkTreeNode(document, IDOMOutlineEntryPtr())
    {
    }

    BookmarkTreeNode(const IDocumentPtr& document, const IDOMOutlineEntryPtr& outline) : m_sourceDocument(document), m_parent(nullptr), m_outline(outline)
    {
    }

    void addChild(const IDOMOutlineEntryPtr& outline);

    IDOMOutlineEntryPtr getChild(const uint32 index) const
    {
        return m_children[index].m_outline;
    }

    void copyNodeTree(const BookmarkTreeNode& currentNode, const IDocumentPtr& targetDocument, int sourceToTargetPageDelta,
        const PageIdToIndexMap& pageIdToPageIndexMap,
        const IDOMOutlineTreeNodePtr& targetOutlineRoot, const IJawsMakoPtr& mako) const;

    static bool bookmarkHasPageId(const IDOMOutlineEntryPtr& outlineEntry, const std::set<DOMid>& pageIds);
    static void buildBookmarkTree(BookmarkTreeNode& root, const std::set<DOMid>& pageIds, const IDOMOutlineTreeNodePtr& outlineRoot);

    IDocumentPtr m_sourceDocument;
    BookmarkTreeNode* m_parent;
    IDOMOutlineEntryPtr m_outline;
    std::vector<BookmarkTreeNode> m_children;
};
