// -----------------------------------------------------------------------
//  <copyright file="BookMarkTreeNode.cpp" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations regarding the use or
//  results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#include "BookMarkTreeNode.h"

#include <jawsmako/jawsmako.h>


BookmarkTreeNode BookmarkTreeNode::createFromDocument(const IDocumentPtr& document, const int startPageIndex, const int endPageIndex)
{
    BookmarkTreeNode root(document);

    IDOMOutlinePtr outline = document->getOutline();
    if (outline)
    {
        std::set<DOMid> pageIds;
        for (int i = startPageIndex; i <= endPageIndex; i++)
            pageIds.insert(document->getPage(i)->getPageId());

        const auto outlineRoot = outline->getOutlineTree()->getRoot();
        buildBookmarkTree(root, pageIds, outlineRoot);
    }

    return root;
}

void BookmarkTreeNode::addChild(const IDOMOutlineEntryPtr& outline)
{
    BookmarkTreeNode child(m_sourceDocument, outline);
    child.m_parent = this;
    m_children.push_back(child);
}

uint32 BookmarkTreeNode::getChildCount(const bool recurse) const
{
    if (!recurse)
        return (uint32)m_children.size();

    uint32 count = 0;
    for (const auto& i : m_children)
        count += 1 + i.getChildCount(true);
    return count;
}

static IDOMPageRectTargetPtr getOutlineRectTarget(const IDOMOutlineEntryConstPtr& outline)
{
    IDOMTargetPtr target;
    return outline->getTarget(target) ? edlobj2IDOMPageRectTarget(target) : IDOMPageRectTargetPtr();;
}

static bool getOutlinePageId(const IDOMOutlineEntryConstPtr& outline, DOMid& pageId)
{
    IDOMPageRectTargetPtr rectTarget = getOutlineRectTarget(outline);
    if (!rectTarget)
        return false;

    pageId = rectTarget->getPageId();
    return true;
}

bool BookmarkTreeNode::bookmarkHasPageId(const IDOMOutlineEntryPtr& outlineEntry, const std::set<DOMid>& pageIds)
{
    DOMid pageId;
    return getOutlinePageId(outlineEntry, pageId) && pageIds.find(pageId) != pageIds.end();
}

void BookmarkTreeNode::buildBookmarkTree(BookmarkTreeNode& root, const std::set<DOMid>& pageIds, const IDOMOutlineTreeNodePtr& outlineRoot)
{
    for (uint32 i = 0; i < outlineRoot->getChildrenCount(); i++)
    {
        IDOMOutlineTreeNodePtr treeNode = outlineRoot->getChild(i);
        IDOMOutlineEntryPtr outlineEntry;
        if (treeNode->getData(outlineEntry) && bookmarkHasPageId(outlineEntry, pageIds))
        {
            root.addChild(outlineEntry);
            buildBookmarkTree(root.m_children[root.m_children.size() - 1], pageIds, treeNode);
        }
    }
}

static PageIdToIndexMap buildPageIdToPageIndexMap(const IDocumentPtr& document)
{
    PageIdToIndexMap map;

    for (uint32 i = 0; i < document->getNumPages(); i++)
    {
        IPagePtr page = document->getPage(i);
        map[page->getPageId()] = i;
        page->release();
    }

    return map;
}

void BookmarkTreeNode::copyNodeTree(const BookmarkTreeNode& currentNode, const IDocumentPtr& targetDocument, const int sourceToTargetPageDelta, const PageIdToIndexMap& pageIdToPageIndexMap, const IDOMOutlineTreeNodePtr& targetOutlineRoot, const IJawsMakoPtr& mako) const
{
    for (const BookmarkTreeNode& childNode : currentNode.m_children)
    {
        IDOMOutlineEntryPtr outline = childNode.m_outline;

        DOMid pageId;
        if (!getOutlinePageId(outline, pageId))
            continue; // Outline does not point to a page.

        const auto mapEntry = pageIdToPageIndexMap.find(pageId);
        if (mapEntry == pageIdToPageIndexMap.end())
            continue; // Outline points to missing page.

        // Clone target, and update page id.
        IDOMPageRectTargetPtr target = getOutlineRectTarget(outline);
        DOMid newPageId = targetDocument->getPage(mapEntry->second + sourceToTargetPageDelta)->getPageId();
        IDOMPageRectTargetPtr clonedTarget = IDOMPageRectTarget::create(mako, newPageId, target->getFitType(), target->getZoom(), target->getLeft(), target->getTop(), target->getRight(), target->getBottom());

        IDOMOutlineEntryPtr clonedOutline = clone(outline, mako);
        clonedOutline->setTarget(clonedTarget);

        IDOMOutlineTreeNodePtr node = createInstance<IDOMOutlineTreeNode>(mako, CClassID(IDOMOutlineTreeNodeClassID));
        if (!node)
            continue;

        node->setData(clonedOutline);
        targetOutlineRoot->appendChild(node);

        copyNodeTree(childNode, targetDocument, sourceToTargetPageDelta, pageIdToPageIndexMap, node, mako);
    }
}

void BookmarkTreeNode::appendToDocument(const IDocumentPtr& targetDocument, const int sourceToTargetPageDelta, const IJawsMakoPtr& mako, const IDOMOutlineTreeNodePtr& targetOutline) const
{
    const auto pageIdToPageIndexMap = buildPageIdToPageIndexMap(m_sourceDocument);
    IDOMOutlineTreeNodePtr targetOutlineRoot;

    // Create target node if none is specified
    if (!targetOutline)
    {
        IDOMOutlinePtr targetOutline = targetDocument->getOutline();
        if (!targetOutline)
        {
            targetOutline = IDOMOutline::create(mako);
            targetDocument->setOutline(targetOutline);
        }

        targetOutlineRoot = targetOutline->getOutlineTree()->getRoot();
    }
    else
        targetOutlineRoot = targetOutline;

    copyNodeTree(*this, targetDocument, sourceToTargetPageDelta, pageIdToPageIndexMap, targetOutlineRoot, mako);
}
