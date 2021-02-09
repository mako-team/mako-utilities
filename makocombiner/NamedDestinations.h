// -----------------------------------------------------------------------
//  <copyright file="NamedDestinations.h" company="Global Graphics Software Ltd">
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

using namespace JawsMako;

class NamedDestinations
{
public:
    NamedDestinations(const IJawsMakoPtr& jawsMako);
    void appendAll(const IDocumentPtr& document);
    void appendRange(const IDocumentPtr& document, uint32 startPageIndex, uint32 endPageIndex);
    CNamedDestinationVect getList() const;

private:
    void append(const INamedDestinationPtr& namedDestination);
    IJawsMakoPtr m_mako;
    CNamedDestinationVect m_destinations;
    CEDLSysStringVect m_names;
    const U8String suffixChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";
};