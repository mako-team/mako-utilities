// -----------------------------------------------------------------------
//  <copyright file="NamedDestinations.cpp" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations regarding the use or
//  results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#include "NamedDestinations.h"

// Constructor
NamedDestinations::NamedDestinations(const IJawsMakoPtr& mako) : m_mako(mako)
{
}

// Records all named destinations in the given document
void NamedDestinations::appendAll(const IDocumentPtr& document)
{
    CNamedDestinationVect namedDestinations = document->getNamedDestinations();
    if (!namedDestinations.empty())
    {
        for (uint32 i = 0; i < namedDestinations.size(); i++)
        {
            append(namedDestinations[i]);
        }
    }
}

// Records named destinations in the given document that refer to pages within the specified range
void NamedDestinations::appendRange(const IDocumentPtr& document, uint32 firstPage, uint32 lastPage)
{
    // Reset the list
    m_destinations.clear();

    // Create list of pageIds to compare against
    CEDLVector<DOMid> pageIds;
    for (uint32 pageIndex = firstPage - 1; pageIndex < lastPage; pageIndex++)
    {
        pageIds.append(document->getPage(pageIndex)->getPageId());
    }

    // Process the named destinations
    CNamedDestinationVect namedDestinations = document->getNamedDestinations();
    if (!namedDestinations.empty())
    {
        for (uint32 i = 0; i < namedDestinations.size(); i++)
        {
            IDOMPageRectTargetPtr target = namedDestinations[i]->getTarget();
            if (target) {
                try
                {
                    uint32 found = pageIds.indexOf(target->getPageId());

                    // There is a matching page, add it to the list
                    append(namedDestinations[i]);
                }
                catch (IError &e)
                {
                }
            }
        }
    }
}

// Appends a named destination avoiding name clashes
void NamedDestinations::append(const INamedDestinationPtr& namedDestination)
{
    U8String name = namedDestination->getName();
    try
    {
        uint32 found = m_names.indexOf(name);

        // name already taken
        U8String suffix = ".  ";
        suffix[1] = suffixChars[rand() % 63];
        suffix[2] = suffixChars[rand() % 63];
        name += suffix;
        const INamedDestinationPtr renamedNamedDestination = INamedDestination::create(m_mako, name, namedDestination->getTarget());
        m_destinations.append(renamedNamedDestination);
        m_names.append(name);
    }
    catch(IError &e)
    {
        // name not found in list
        m_destinations.append(namedDestination);
        m_names.append(name);
    }
}

// Return named destinations
CNamedDestinationVect NamedDestinations::getList() const
{
    return m_destinations;
}