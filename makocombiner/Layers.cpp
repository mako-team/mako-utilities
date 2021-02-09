// -----------------------------------------------------------------------
//  <copyright file="Layers.cpp" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations regarding the use or
//  results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#include "Layers.h"

// Search content groups for a given group reference
bool findGroupReference(const COptionalContentGroupVect& optionalContentGroups,
    const IOptionalContentGroupReferencePtr& groupReference,
    IOptionalContentGroupPtr& optionalContentGroup)
{
    for (uint32 ocgIndex = 0; ocgIndex < optionalContentGroups.size(); ocgIndex++)
    {
        if (optionalContentGroups[ocgIndex]->getReference()->equals(groupReference))
        {
            optionalContentGroup = optionalContentGroups[ocgIndex];
            return true;
        }
    }
    return false;
}

// Constructor
Layers::Layers(const IJawsMakoPtr& mako) : m_mako(mako)
{
    m_newOptionalContent = IOptionalContent::create(m_mako);
    m_newConfiguration = m_newOptionalContent->getDefaultConfiguration();
    m_newOrderEntryVect = m_newConfiguration->getOrder();
}

// Get the OCGs from the PDF
bool Layers::AppendDocumentLayers(const IDocumentPtr& sourceDocument, const U8String name)
{
    // Is there optional content in the source document?
    const IOptionalContentPtr optionalContent = sourceDocument->getOptionalContent();
    if (optionalContent)
    {
        // Copy over the groups
        COptionalContentGroupVect groups = optionalContent->getGroups();
        for (uint32 groupIndex = 0; groupIndex < groups.size(); groupIndex++)
        {
            m_newOptionalContent->addGroup(groups[groupIndex]->clone(), sourceDocument);
        }
        
        //// A vector to hold the order entries copied from the source
        //IOptionalContentConfiguration::COrderEntryVect subVect;

        //// An order entry 
        //IOptionalContentConfiguration::COrderEntryPtr orderEntry;

        //// Get the source document optional content groups
        //const COptionalContentGroupVect optionalContentGroups = optionalContent->getGroups();

        //// Use the Order array, if present, to control which layers are copied
        //const IOptionalContentConfiguration::COrderEntryVect order = optionalContent->getDefaultConfiguration()->getOrder();
        //IOptionalContentGroupPtr optionalContentGroup;
        //IOptionalContentGroupReferencePtr groupReference;
        //
        //if (!order.empty())
        //    for (uint32 orderIndex = 0; orderIndex < order.size(); orderIndex++)
        //    {
        //        // If it's a group, then add it to the list
        //        if (order[orderIndex]->isGroup)
        //        {
        //            groupReference = order[orderIndex]->groupRef;
        //            if (findGroupReference(optionalContentGroups, groupReference, optionalContentGroup))
        //            {
        //                orderEntry = IOptionalContentConfiguration::COrderEntry::create();
        //                orderEntry->groupRef = groupReference;
        //                orderEntry->isGroup = true;
        //                subVect.append(orderEntry);
        //            }
        //        }
        //        else
        //        {
        //            // If it's a parent group, add its children to the list
        //            const IOptionalContentConfiguration::COrderEntryVect children = order[orderIndex]->children;
        //            for (uint32 childIndex = 0; childIndex < children.size(); childIndex++)
        //            {
        //                groupReference = children[childIndex]->groupRef;
        //                if (findGroupReference(optionalContentGroups, groupReference, optionalContentGroup))
        //                {
        //                    orderEntry = IOptionalContentConfiguration::COrderEntry::create();
        //                    orderEntry->groupRef = groupReference;
        //                    orderEntry->isGroup = true;
        //                    subVect.append(orderEntry);
        //                }
        //            }
        //        }
        //    }
        //else
        //{
        //    // In the unlikely event that there is no order array present
        //    for (uint32 ocgIndex = 0; ocgIndex < optionalContentGroups.size(); ocgIndex++)
        //    {
        //        groupReference = optionalContentGroups[ocgIndex]->getReference();
        //        if (findGroupReference(optionalContentGroups, groupReference, optionalContentGroup))
        //        {
        //            orderEntry = IOptionalContentConfiguration::COrderEntry::create();
        //            orderEntry->groupRef = groupReference;
        //            orderEntry->isGroup = true;
        //            subVect.append(orderEntry);
        //        }
        //    }
        //}

        // Add a new parent OCG to the target order vector
        IOptionalContentConfiguration::COrderEntryPtr newOrderEntry = IOptionalContentConfiguration::COrderEntry::create();
        newOrderEntry->isGroup = false;
        //newOrderEntry->children = subVect;
        newOrderEntry->children = optionalContent->getDefaultConfiguration()->getOrder();
        newOrderEntry->name = name;
        m_newOrderEntryVect.append(newOrderEntry);
        m_newConfiguration->setOrder(m_newOrderEntryVect);
        m_newConfiguration->setListMode(IOptionalContentConfiguration::eLMAllPages);
    }
    return true;
}

// Return layers
IOptionalContentPtr Layers::getLayers()
{
    return m_newOptionalContent;
}