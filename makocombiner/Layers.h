// -----------------------------------------------------------------------
//  <copyright file="Layers.h" company="Global Graphics Software Ltd">
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

class Layers
{
public:
    Layers(const IJawsMakoPtr& mako);
    bool AppendDocumentLayers(const IDocumentPtr& sourceDocument, U8String name);
    IOptionalContentPtr getLayers();

private:
    IJawsMakoPtr m_mako;
    IOptionalContentPtr m_newOptionalContent;
    IOptionalContentConfigurationPtr m_newConfiguration;
    IOptionalContentConfiguration::COrderEntryVect m_newOrderEntryVect;
};