// -----------------------------------------------------------------------
//  <copyright file="MakoPageSizes.h" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations regarding the use or
//  results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#pragma once
#include <map>

using namespace JawsMako;
using namespace EDL;

struct sPageSize
{
    double width;
    double height;
};

inline std::map<String, sPageSize> GetPageSizeList() {
    
    std::map<String, sPageSize> pageSizes;
    pageSizes.insert(std::make_pair(L"LETTER", sPageSize{ 816.0, 1056.0 }));
    pageSizes.insert(std::make_pair(L"TABLOID", sPageSize{ 1056.0, 1632.0 }));
    pageSizes.insert(std::make_pair(L"LEDGER", sPageSize{ 1632.0, 1056.0 }));
    pageSizes.insert(std::make_pair(L"LEGAL", sPageSize{ 816.0, 1344.0 }));
    pageSizes.insert(std::make_pair(L"STATEMENT", sPageSize{ 528.0, 816.0 }));
    pageSizes.insert(std::make_pair(L"EXECUTIVE", sPageSize{ 696.0, 1008.0 }));
    pageSizes.insert(std::make_pair(L"A3", sPageSize{ 1122.5, 1587.4 }));
    pageSizes.insert(std::make_pair(L"A4", sPageSize{ 793.7, 1122.5 }));
    pageSizes.insert(std::make_pair(L"A4SMALL", sPageSize{ 793.7, 1122.5 }));
    pageSizes.insert(std::make_pair(L"A5", sPageSize{ 559.4, 793.7 }));
    pageSizes.insert(std::make_pair(L"B4", sPageSize{ 971.3, 1375.7 }));
    pageSizes.insert(std::make_pair(L"B5", sPageSize{ 687.8, 971.3 }));
    pageSizes.insert(std::make_pair(L"FOLIO", sPageSize{ 816.0, 1248.0 }));
    pageSizes.insert(std::make_pair(L"QUARTO", sPageSize{ 812.6, 1039.4 }));
    pageSizes.insert(std::make_pair(L"10X14", sPageSize{ 960.0, 1344.0 }));
    pageSizes.insert(std::make_pair(L"11X17", sPageSize{ 1056.0, 1632.0 }));
    pageSizes.insert(std::make_pair(L"NOTE", sPageSize{ 816.0, 1056.0 }));
    pageSizes.insert(std::make_pair(L"ENV_9", sPageSize{ 372.0, 852.0 }));
    pageSizes.insert(std::make_pair(L"ENV_10", sPageSize{ 396.0, 912.0 }));
    pageSizes.insert(std::make_pair(L"ENV_11", sPageSize{ 432.0, 996.0 }));
    pageSizes.insert(std::make_pair(L"ENV_12", sPageSize{ 456.0, 1056.0 }));
    pageSizes.insert(std::make_pair(L"ENV_14", sPageSize{ 480.0, 1104.0 }));
    pageSizes.insert(std::make_pair(L"CSHEET", sPageSize{ 1632.0, 2112.0 }));
    pageSizes.insert(std::make_pair(L"DSHEET", sPageSize{ 2112.0, 3264.0 }));
    pageSizes.insert(std::make_pair(L"ESHEET", sPageSize{ 3264.0, 4224.0 }));
    pageSizes.insert(std::make_pair(L"ENV_DL", sPageSize{ 415.7, 831.4 }));
    pageSizes.insert(std::make_pair(L"ENV_C5", sPageSize{ 612.2, 865.4 }));
    pageSizes.insert(std::make_pair(L"ENV_C3", sPageSize{ 1224.6, 1731.0 }));
    pageSizes.insert(std::make_pair(L"ENV_C4", sPageSize{ 865.4, 1224.6 }));
    pageSizes.insert(std::make_pair(L"ENV_C6", sPageSize{ 430.8, 612.2 }));
    pageSizes.insert(std::make_pair(L"ENV_C65", sPageSize{ 430.8, 865.4 }));
    pageSizes.insert(std::make_pair(L"ENV_B4", sPageSize{ 944.9, 1334.2 }));
    pageSizes.insert(std::make_pair(L"ENV_B5", sPageSize{ 665.1, 944.9 }));
    pageSizes.insert(std::make_pair(L"ENV_B6", sPageSize{ 665.1, 472.4 }));
    pageSizes.insert(std::make_pair(L"ENV_ITALY", sPageSize{ 415.7, 869.3 }));
    pageSizes.insert(std::make_pair(L"ENV_MONARCH", sPageSize{ 372.0, 720.0 }));
    pageSizes.insert(std::make_pair(L"ENV_PERSONAL", sPageSize{ 348.0, 624.0 }));
    pageSizes.insert(std::make_pair(L"FANFOLD_US", sPageSize{ 1428.0, 1056.0 }));
    pageSizes.insert(std::make_pair(L"FANFOLD_STD_GERMAN", sPageSize{ 816.0, 1152.0 }));
    pageSizes.insert(std::make_pair(L"FANFOLD_LGL_GERMAN", sPageSize{ 816.0, 1248.0 }));
    pageSizes.insert(std::make_pair(L"ISO_B4", sPageSize{ 944.9, 1334.2 }));
    pageSizes.insert(std::make_pair(L"JAPANESE_POSTCARD", sPageSize{ 377.9, 559.4 }));
    pageSizes.insert(std::make_pair(L"9X11", sPageSize{ 864.0, 1056.0 }));
    pageSizes.insert(std::make_pair(L"10X11", sPageSize{ 960.0, 1056.0 }));
    pageSizes.insert(std::make_pair(L"15X11", sPageSize{ 1440.0, 1056.0 }));
    pageSizes.insert(std::make_pair(L"ENV_INVITE", sPageSize{ 831.4, 831.4 }));
    pageSizes.insert(std::make_pair(L"LETTER_EXTRA", sPageSize{ 912.0, 1152.0 }));
    pageSizes.insert(std::make_pair(L"LEGAL_EXTRA", sPageSize{ 912.0, 1440.0 }));
    pageSizes.insert(std::make_pair(L"TABLOID_EXTRA", sPageSize{ 1152.0, 1728.0 }));
    pageSizes.insert(std::make_pair(L"A4_EXTRA", sPageSize{ 889.9, 1218.2 }));
    pageSizes.insert(std::make_pair(L"A_PLUS", sPageSize{ 857.9, 1345.4 }));
    pageSizes.insert(std::make_pair(L"B_PLUS", sPageSize{ 1152.7, 1840.6 }));
    pageSizes.insert(std::make_pair(L"LETTER_PLUS", sPageSize{ 816.0, 1218.2 }));
    pageSizes.insert(std::make_pair(L"A4_PLUS", sPageSize{ 793.7, 1247.2 }));
    pageSizes.insert(std::make_pair(L"A3_EXTRA", sPageSize{ 1217.0, 1681.8 }));
    pageSizes.insert(std::make_pair(L"A5_EXTRA", sPageSize{ 657.6, 888.2 }));
    pageSizes.insert(std::make_pair(L"B5_EXTRA", sPageSize{ 759.7, 1043.1 }));
    pageSizes.insert(std::make_pair(L"A2", sPageSize{ 1587.4, 2245.0 }));
    pageSizes.insert(std::make_pair(L"DBL_JAPANESE_POSTCARD", sPageSize{ 755.8, 559.4 }));
    pageSizes.insert(std::make_pair(L"A6", sPageSize{ 396.8, 559.4 }));
    pageSizes.insert(std::make_pair(L"JENV_KAKU2", sPageSize{ 907.0, 1254.8 }));
    pageSizes.insert(std::make_pair(L"JENV_KAKU3", sPageSize{ 816.3, 1046.9 }));
    pageSizes.insert(std::make_pair(L"JENV_CHOU3", sPageSize{ 453.5, 888.2 }));
    pageSizes.insert(std::make_pair(L"JENV_CHOU4", sPageSize{ 340.1, 774.8 }));
    pageSizes.insert(std::make_pair(L"B6_JIS", sPageSize{ 483.8, 687.8 }));
    pageSizes.insert(std::make_pair(L"12X11", sPageSize{ 1152.5, 1056.4 }));
    pageSizes.insert(std::make_pair(L"JENV_YOU4", sPageSize{ 396.8, 888.2 }));
    pageSizes.insert(std::make_pair(L"P16K", sPageSize{ 710.5, 982.6 }));
    pageSizes.insert(std::make_pair(L"P32K", sPageSize{ 491.3, 695.4 }));
    pageSizes.insert(std::make_pair(L"P32KBIG", sPageSize{ 529.1, 767.2 }));
    pageSizes.insert(std::make_pair(L"PENV_1", sPageSize{ 385.4, 623.6 }));
    pageSizes.insert(std::make_pair(L"PENV_2", sPageSize{ 385.4, 665.1 }));
    pageSizes.insert(std::make_pair(L"PENV_3", sPageSize{ 472.4, 665.1 }));
    pageSizes.insert(std::make_pair(L"PENV_4", sPageSize{ 415.7, 786.1 }));
    pageSizes.insert(std::make_pair(L"PENV_5", sPageSize{ 415.7, 831.4 }));
    pageSizes.insert(std::make_pair(L"PENV_6", sPageSize{ 453.5, 869.3 }));
    pageSizes.insert(std::make_pair(L"PENV_7", sPageSize{ 604.7, 869.3 }));
    pageSizes.insert(std::make_pair(L"PENV_8", sPageSize{ 453.5, 1167.8 }));
    pageSizes.insert(std::make_pair(L"PENV_9", sPageSize{ 865.4, 1224.6 }));
    pageSizes.insert(std::make_pair(L"PENV_10", sPageSize{ 1224.6, 1731.0 }));
    return pageSizes;
}
