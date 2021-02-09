// -----------------------------------------------------------------------
//  <copyright file="makocombiner.cpp" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations regarding the use or
//  results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#include <algorithm>
#include <exception>
#include <codecvt>
#include <fstream>
#include <iostream>

#include <fcntl.h>
#include <stdlib.h>
#include <stdexcept>
#include <wctype.h>
#include <jawsmako/jawsmako.h>
#include <jawsmako/pdfoutput.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <corecrt_io.h>
#include <direct.h>
#endif

#include "BookMarkTreeNode.h"
#include "NamedDestinations.h"
#include "Layers.h"
#include <edl/idommetadata.h>

using namespace JawsMako;
using namespace EDL;

// Controls if appendPage() is used with a source document parameter
// If true, processing is slower but the copying of the page more thorough, eg copying bookmarks and form field metadata
static const bool deepCopy = false;

struct sPageRange
{
    uint32 firstPage = 1;
    uint32 lastPage = 0;
};

struct sArgument
{
    String fullPath;
    String basename;
    String ext;
    String modifier;
    eFileFormat fileFormat;
    std::vector<sPageRange> pageRanges;
};

static void usage()
{
    std::wcout << "Mako Combiner(single thread) v1.2.0" << std::endl << std::endl;
    std::wcout << L"Usage:" << std::endl;
    std::wcout << L"   makocombiner <source file 1.xxx> <source file 2.xxx> .. <source file n.xxx>" << std::endl;
    std::wcout << L"                Combines (merges) multiple files into a single file." << std::endl;
    std::wcout << L"                Bookmarks and/or named destinations in the source are copied to the output." << std::endl;
    std::wcout << L"                OCGs (layer information) in the source is copied to the output." << std::endl;
    std::wcout << L"Parameters:" << std::endl;
    std::wcout << L"                <source file.xxx> where .xxx can be any of .pdf, .xps, .pxl (PCL/XL) or .pcl (PCL5)" << std::endl;
    std::wcout << L"                /n-m[;n-m]... indicates one or more page ranges to copy, eg 10-20;80;90-" << std::endl;
    std::wcout << L"                  - A range of n- means from n to end." << std::endl;
    std::wcout << L"                  - Invalid page ranges are adjusted automatically or ignored." << std::endl;
    std::wcout << L"                <filename>/o indicates the file is the output file." << std::endl;
    std::wcout << L"                If no output file is declared, a default of 'Combined.xxx' will be used (where xxx matches the first named file)." << std::endl;
    std::wcout << L" -or-" << std::endl;
    std::wcout << L"   makocombiner <source file list (text file)> [<output file>] (to combine a list of files into the output file)" << std::endl;
}

bool isSeparator(std::wistream& source, const wchar_t separ)
{
    wchar_t next;
    source >> next;
    if (source && next != separ) {
        source.putback(next);
    }
    return source && next == separ;
}

// Get file extension (in lower case)
static String getExtension(const String& path)
{
    // Get the extension in lower case
    if (path.size() < 4)
    {
        // Cannot determine the extension if there isn't one!
        std::string message("Cannot determine file extension for path ");
        message += StringToU8String(path).c_str();
        throw std::length_error(message);
    }

    const size_t extensionPosition = path.find_last_of('.');
    String extension = path.substr(extensionPosition);
    std::transform(extension.begin(), extension.end(), extension.begin(), towlower);
    return extension;
}

// Return file extension for given file format
static String extensionFromFormat(eFileFormat fmt)
{
    if (fmt == eFFPDF)
        return L".pdf";
    if (fmt == eFFXPS)
        return L".xps";
    if (fmt == eFFPCLXL)
        return L".pxl";
    if (fmt == eFFPCL5)
        return L".pcl";
    return L"";
}

// Determine the associated format for a given ext from the file extension
static eFileFormat formatFromExtension(const String& ext)
{
    if (ext == L".pdf")
        return eFFPDF;
    if (ext == L".xps")
        return eFFXPS;
    if (ext == L".pxl")
        return eFFPCLXL;
    if (ext == L".pcl")
        return eFFPCL5;
    return eFFUnknown;
}

// Process a page range to add to the list of page ranges
void processRange(std::vector<sPageRange>& results, std::wistream& source)
{
    sPageRange pageRange;
    source >> pageRange.firstPage;
    //pageRange.lastPage = pageRange.firstPage;
    if (isSeparator(source, '-'))
        source >> pageRange.lastPage;
    else
        pageRange.lastPage = pageRange.firstPage;

    if (pageRange.lastPage && pageRange.lastPage < pageRange.firstPage) {
        const uint32 p = pageRange.firstPage;
        pageRange.firstPage = pageRange.lastPage;
        pageRange.lastPage = p;
    }
    if (pageRange.firstPage)
        results.push_back(pageRange);
}

// Split a command argument into parts
static sArgument split_argument(const String &path)
{
    // Check path size; must be 5 characters or greater
    if (path.size() < 5)
    {
        // Cannot determine the extension if there isn't one!
        std::string message("Cannot determine file extension for path ");
        message += StringToU8String(path).c_str();
        throw std::length_error(message);
    }

    const size_t extensionPosition = path.find_last_of('.');
    const size_t modifierPosition = path.find_last_of('/');
    String extension;
    sArgument argument;
    const sPageRange emptyPageRange;
    
    if (extensionPosition != String::npos) {
        argument.basename = path.substr(0, extensionPosition);
        if (modifierPosition != String::npos) {
            if (modifierPosition > extensionPosition) {
                extension = path.substr(extensionPosition, modifierPosition - extensionPosition);
                if (path.size() > modifierPosition) {
                    String modifier = path.substr(modifierPosition + 1, path.size() - modifierPosition);
                    std::transform(modifier.begin(), modifier.end(), modifier.begin(), towlower);
                    argument.modifier = modifier.substr(0, 1);
                    if (modifier.length() > 1)    // Page range?
                    {
                        std::wistringstream argument_ws(modifier.c_str());
                        processRange(argument.pageRanges, argument_ws);
                        while (isSeparator(argument_ws, ';')) {
                            processRange(argument.pageRanges, argument_ws);
                        }
                    }
                    //else
                    //    argument.pageRanges.push_back(emptyPageRange);
                }
            }
        }
        else {
            extension = path.substr(extensionPosition);
        }
        std::transform(extension.begin(), extension.end(), extension.begin(), towlower);
        argument.ext = extension;
        argument.fileFormat = formatFromExtension(extension);
        argument.fullPath = argument.basename + argument.ext;
    }
    return argument;
}

// Check if file exists
// Assumes that it does, unless stat fails with ENOENT.
static bool fileExists(const String &path)
{
    // Use stat to check existence of a file
#ifdef _WIN32
    struct _stat statBuff;
    if (_wstat(path.c_str(), &statBuff) == -1 && errno == ENOENT)
    {
        return false;
    }
#else
    struct stat statBuff;
    if (stat(StringToU8String(path).c_str(), &statBuff) == -1 && errno == ENOENT)
    {
        return false;
    }
#endif
    // Assume it exists
    return true;
}

// Return filename without preceding path
static String filenameWithoutPrecedingPath(const String &path)
{
    std::string filepath = StringToU8String(path).c_str();
    const size_t lastPathSeparator = filepath.find_last_of(PATH_SEP_CHAR);
    if (lastPathSeparator != String::npos)
        return U8StringToString(filepath.substr(lastPathSeparator + 1).c_str());
    return U8StringToString(filepath.c_str());
}

// Create a new outline (bookmark) node with a description and target
IDOMOutlineTreeNodePtr makeOutlineNode(IJawsMakoPtr jawsMako, uint32 pageIndex, String entry)
{
    // Create a target linking to the whole page
    IDOMPageTargetPtr pageTarget = createInstance<IDOMPageTarget>(jawsMako, CClassID(IDOMPageTargetClassID));
    pageTarget->setTargetPage(pageIndex + 1);

    // Use a blue color
    IDOMColorPtr blueColor = IDOMColor::create(jawsMako, IDOMColorSpaceDeviceRGB::create(jawsMako), 1.0, 0.09, 0.6, 0.89);

    // Create the outline node
    IDOMOutlineTreeNodePtr newNode = IDOMOutlineEntry::createNode(jawsMako,
        entry,
        true,
        pageTarget,
        blueColor,
        IDOMOutlineEntry::eTextStyleBold);
    return newNode;
}

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
{
    _setmode(_fileno(stderr), _O_U16TEXT);
    _setmaxstdio(2048);
#else
int main(int argc, char *argv[])
{
#endif

    try
    {
        // Create our JawsMako instance.
        IJawsMakoPtr jawsMako = IJawsMako::create();
        IJawsMako::enableAllFeatures(jawsMako);

        // Strings to hold arguments
        String outputFilePath;
        eFileFormat outputFileFormat = eFFUnknown;

        // Vector to hold list of files to be processed
        CEDLVector<sArgument> inputFileList;

        // Check number of arguments
        if (argc < 2)
        {
            usage();
            return 1;
        }

        // ReSharper disable once CppJoinDeclarationAndAssignment
        String arg;
        bool fileListDetected = false;

        // Process arguments
        for (uint16 i = 1; i < argc; i++)
        {
#ifdef _WIN32
            // ReSharper disable once CppJoinDeclarationAndAssignment
            arg = argv[i];
#else
            arg = U8StringToString(U8String(argv[i]));
#endif    
            sArgument argument = split_argument(arg);

            // Add a PDF to the list of files to be processed, unless it's the output file
            if (argument.fileFormat != eFFUnknown)
            {
                if (argument.modifier == L"o" || fileListDetected)
                {
                    outputFilePath = argument.fullPath;
                    outputFileFormat = argument.fileFormat;
                    fileListDetected = false;
                }
                else
                {
                    if (fileExists(argument.fullPath))
                    {
                        inputFileList.append(argument);
                    }
                }
            }

            // Read an input list of files to be processed. UTF8 supported
            if (argument.ext == L".txt")
            {
                outputFileFormat = eFFPDF;
                outputFilePath = argument.basename + extensionFromFormat(outputFileFormat);
                fileListDetected = true;    // next arg is the output file (if there is one)

                typedef std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode::consume_header> MyCodeCvt;
#ifdef _WIN32
                std::wifstream myStream(arg.c_str());
#else
                std::wifstream myStream(StringToU8String(arg));
#endif
                std::locale myLocale(myStream.getloc(), new MyCodeCvt());
                myStream.imbue(myLocale);
                
                wchar_t* line = new wchar_t[MAXPATHLEN];
                myStream.getline(line, MAXPATHLEN);
                while (wcslen(line)) {
                    if (fileExists(line))
                    {
                        sArgument lineArg;
                        lineArg.fullPath = line;
                        lineArg.fileFormat = formatFromExtension(getExtension(String(line)));
                        inputFileList.append(lineArg);
                    }
                    myStream.getline(line, MAXPATHLEN);
                }
                myStream.close();
                delete[] line;
            }
        }

        // Use a default output filename, avoiding overwriting
        if (outputFilePath.size() == 0)
        {
            const String outputFileBase = L"Combined";
            outputFilePath = outputFileBase + inputFileList[0].ext;
            outputFileFormat = inputFileList[0].fileFormat;
            uint16 i = 1;
            while(fileExists(outputFilePath))
            {
                outputFilePath = String(outputFileBase + std::to_wstring(i++).c_str() + inputFileList[0].ext);
            }
        }        
        
        if (inputFileList.empty())
        {
            usage();
            throw std::invalid_argument("\n   The input file list is empty. \n   This may be because the filenames cannot be read from the text file, or that the files cannot be found.");
        }

        // Timer
        const clock_t begin = clock();

        // OUTPUT: Create an empty assembly, document, outline, named destinations list and optional content 
        IDocumentAssemblyPtr assembly = IDocumentAssembly::create(jawsMako);
        IDocumentPtr document = IDocument::create(jawsMako);
        IDOMOutlinePtr destOutline = IDOMOutline::create(jawsMako);
        document->setOutline(destOutline); // new 4.3 API
        assembly->appendDocument(document);
        NamedDestinations namedDestinations(jawsMako);
        Layers layers(jawsMako);

        // Process each of the input documents
        for (uint32 i = 0; i < inputFileList.size() && i < 2048; i++)
        {
            // INPUT: Create a PDF input
            IInputPtr input = IInput::create(jawsMako, inputFileList[i].fileFormat);
            IDocumentPtr sourceDocument = input->open(inputFileList[i].fullPath)->getDocument();
            IDOMOutlinePtr sourceOutline = sourceDocument->getOutline();
            std::wcout << L"Processing \'";
            std::wcerr << inputFileList[i].fullPath;
            std::wcout << L"\'...";
            std::wcerr << std::endl;

            // Save the position of where the appended document begins
            uint32 targetDocumentPageIndex = document->getNumPages();

            // Copy pages
            const uint32 pageCount = sourceDocument->getNumPages();
            if (!inputFileList[i].pageRanges.size())
            {
                // Create default page range
                sPageRange dpr;
                dpr.lastPage = pageCount;
                inputFileList[i].pageRanges.emplace_back(dpr);
            }

            // Create a bookmark for the document
            IDOMOutlineTreeNodePtr newNode = makeOutlineNode(jawsMako, targetDocumentPageIndex, filenameWithoutPrecedingPath(inputFileList[i].fullPath));
            document->getOutline()->getOutlineTree()->getRoot()->appendChild(newNode);

            // Process each of the associated page ranges
            int x = 0;
            for (uint32 j = 0; j < inputFileList[i].pageRanges.size(); ++j, ++x)
            {
                // Adjust out of range page numbers
                if (inputFileList[i].pageRanges[j].firstPage > pageCount)
                    inputFileList[i].pageRanges[j].firstPage = pageCount;

                // A last page number of zero means until the end
                if (inputFileList[i].pageRanges[j].lastPage == 0 || inputFileList[i].pageRanges[j].lastPage > pageCount)
                    inputFileList[i].pageRanges[j].lastPage = pageCount;

                const uint32 sourceFirstPageIndex = inputFileList[i].pageRanges[j].firstPage - 1;
                const uint32 sourceLastPageIndex = inputFileList[i].pageRanges[j].lastPage - 1;

                // Copy pages
                for (uint32 pageIndex = sourceFirstPageIndex; pageIndex < inputFileList[i].pageRanges[j].lastPage; pageIndex++)
                {
                    IPagePtr sourcePage = sourceDocument->getPage(pageIndex);
                    if (!deepCopy)
                        document->appendPage(sourcePage);
                    else
                        document->appendPage(sourcePage, sourceDocument); 
                    sourcePage->release();
                }

                // Copy bookmarks (not needed if a deep copy is specified, as that copies bookmarks automatically)
                if (!deepCopy)
                {
                    BookmarkTreeNode sourceBookmarks = BookmarkTreeNode::createFromDocument(sourceDocument, sourceFirstPageIndex, sourceLastPageIndex);
                    if (sourceBookmarks.getChildCount(true))
                        sourceBookmarks.appendToDocument(document, targetDocumentPageIndex - sourceFirstPageIndex, jawsMako, newNode);
                }

                // Move up the start position in the target document for the next range of pages
                targetDocumentPageIndex += sourceLastPageIndex - sourceFirstPageIndex + 1;
            }

            // Append named destinations in the source to the target (PDF only)
            if (outputFileFormat == eFFPDF)
                namedDestinations.appendAll(sourceDocument);

            // Append OCG information (layers) (PDF only)
            if (inputFileList[i].fileFormat == eFFPDF && (outputFileFormat == eFFPDF))
                layers.AppendDocumentLayers(sourceDocument, StringToU8String(inputFileList[i].basename));
        }

        // Set new Named Destinations (PDF only)
        if (outputFileFormat == eFFPDF)
            document->setNamedDestinations(namedDestinations.getList()); // new 4.6 API

        // Set the viewer preferences so that the outline is visible when the file is opened (PDF only)
        if (outputFileFormat == eFFPDF)
        {
            IDOMMetadataPtr metadata = IDOMMetadata::create(jawsMako);
            if (metadata->setProperty(IDOMMetadata::ePageView, "PageMode", PValue(String(L"UseOutlines"))))
                assembly->setJobMetadata(metadata);
            else
                std::cout << "Could not set PDF viewer preferences" << std::endl;
        }

        // Add copied layer information (PDF only)
        if (outputFileFormat == eFFPDF)
            document->setOptionalContent(layers.getLayers());

        // Now we can write this out
        std::wcout << L"Writing \'";
        std::wcerr << outputFilePath;
        std::wcout << L"\'...";
        std::wcerr << std::endl;
        IOutputPtr output = IOutput::create(jawsMako, outputFileFormat);
        output->writeAssembly(assembly, outputFilePath);

        const clock_t end = clock();
        const double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        std::wcout << L"Elapsed time: " << elapsed_secs << L" seconds." << std::endl;
    }
    catch (IError &e)
    {
        String errorFormatString = getEDLErrorString(e.getErrorCode());
        std::wcerr << L"Exception: " << e.getErrorDescription(errorFormatString) << std::endl;
        return e.getErrorCode();
    }
    catch (std::exception &e)
    {
        std::wcerr << L"std::exception thrown: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}