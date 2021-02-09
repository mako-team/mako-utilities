// -----------------------------------------------------------------------
//  <copyright file="makoimposer.cpp" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations regarding the use or
//  results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#include <exception>
#include <iostream>
#include <stdexcept>
#include <wctype.h>
#include <jawsmako/jawsmako.h>
#include <jawsmako/pdfinput.h>
#include <jawsmako/pdfoutput.h>
#include "MakoPageSizes.h"
#include <algorithm>
#include <jawsmako/xpsoutput.h>
#include <edl/idommetadata.h>

#ifdef _WIN32
#include <fcntl.h>
#include <corecrt_io.h>
#endif

using namespace JawsMako;
using namespace EDL;

struct sParameters
{
    String inputFullPath;
    String inputBasename;
    eFileFormat inputType;
    U8String userPassword;
    String outputPath;
    String outputBasename;
    eFileFormat outputType;
    String outputFullPath;
    bool flattenTransparency;
    bool simulateOverprint;
    bool sequential;
    double spreadWidth;
    double spreadHeight;
};

static void usage(std::map<String, sPageSize> pageSizes)
{
    std::wcout << "Mako Imposer v1.2.0\n" << std::endl;
    std::wcout << L"Usage:" << std::endl;
    std::wcout << L"   makoimposer input.pdf|xps|pxl|pcl [output.pdf|xps|pxl|pcl] [parameter=setting] [parameter=setting] ..." << std::endl;;
    std::wcout << L"   parameter=setting  one or more settings, described below." << std::endl;
    std::wcout << std::endl;
    std::wcout << L"Parameters:" << std::endl;
    std::wcout << L"   input.xxx      source file from which to extract pages, where xxx is pdf, xps, pxl (PCL/XL) or pcl (PCL5)" << std::endl;
    std::wcout << L"   output.yyy     target file to write the output to, where yyy is pdf, xps, pxl or pcl." << std::endl;
    std::wcout << L"                    If no output file is declared, <input>_booklet.pdf is assumed." << std::endl;
    std::wcout << L"   pw=<password>  PDF password, if required to open the file." << std::endl;
    std::wcout << L"   f=yes|no       Flatten transparency. Default is no, ie retain transparency as is." << std::endl;
    std::wcout << L"   o=yes|no       Simulate overprint. Default is no, ie do not simulate overprint." << std::endl;
    std::wcout << L"   s=yes|no       Impose pages sequentially. Default is no, ie use booklet imposition" << std::endl;
    std::wcout << L"   p=pagesize     Page size chosen from the list below. Default is the size of a double page spread." << std::endl;
    std::wcout << std::endl;

    uint8 colCount = 0;
    std::map<String, sPageSize>::iterator it = pageSizes.begin();
    while (it != pageSizes.end())
    {
        String name = it->first;
        std::wcout << name << L"\t";
        if (name.size() < 16) {
            std::wcout << L"\t";
        }
        if (name.size() < 8) {
            std::wcout << L"\t";
        }
        ++it;
        if (++colCount == 4)
        {
            std::wcout << std::endl;
            colCount = 0;
        }
    }
}

// Return filename without preceding path
static String filenameWithoutPrecedingPath(const String &path)
{
	const std::string filepath = StringToU8String(path).c_str();
    const size_t lastPathSeparator = filepath.find_last_of(PATH_SEP_CHAR);
    if (lastPathSeparator != String::npos)
        return U8StringToString(filepath.substr(lastPathSeparator + 1).c_str());
    return U8StringToString(filepath.c_str());
}

// Return preceding path without filename
static String precedingPathWithoutFilename(const String &path)
{
    std::string filepath = StringToU8String(path).c_str();
    const size_t lastPathSeparator = filepath.find_last_of(PATH_SEP_CHAR);
    if (lastPathSeparator != String::npos)
        return U8StringToString(filepath.substr(0, lastPathSeparator + 1).c_str());
    return L"";
}

// Return basename only 
static String basename(const String &path)
{
    std::string filepath = StringToU8String(filenameWithoutPrecedingPath(path)).c_str();
    size_t extensionPosition = filepath.find_last_of('.');
    if (extensionPosition != String::npos)
        return U8StringToString(filepath.substr(0, extensionPosition).c_str());
    return path;
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

// Determine the associated format for a given path from the file extension
static eFileFormat fileFormatFromPath(const String& path)
{
    const String extension = getExtension(path);

    if (extension == L".ps" || extension == L".eps")
        return eFFPS;
    if (extension == L".pdf")
        return eFFPDF;
    if (extension == L".xps")
        return eFFXPS;
    if (extension == L".pxl")
        return eFFPCLXL;
    if (extension == L".pcl")
        return eFFPCL5;

    std::string message("Unsupported file type for (input) path ");
    message += StringToU8String(path).c_str();
    throw std::invalid_argument(message);
}

// Return file extension for given file format
static String extensionFromFormat(eFileFormat fmt)
{
    if (fmt == eFFPDF)
        return L".pdf";
    if (fmt == eFFXPS)
        return L".xps";
    if (fmt == eFFPS)
        return L".ps";
    if (fmt == eFFPCLXL)
        return L".pxl";
    if (fmt == eFFPCL5)
        return L".pcl";
    return L"";
}

// Populate params structure with items specified on the command line
static sParameters parse_params(CEDLStringVect arguments, std::map<String, sPageSize> pageSizes)
{
    sParameters params;

    // Set defaults (A3)
    params.userPassword = "";
    params.inputType = eFFPDF;
    params.outputType = eFFPDF;
    params.spreadHeight = 0;
    params.spreadWidth = 0;
    params.sequential = false;
    params.simulateOverprint = false;
    params.flattenTransparency = false;

    for (uint32 i = 0; i < arguments.size(); i++)
    {
        const size_t equalsPos = arguments[i].find('=');
        if (equalsPos == String::npos)
        {
            // A filename; first is input, second is output
            if (params.inputFullPath.length() == 0)
            {
                params.inputFullPath = arguments[i];
                params.inputType = fileFormatFromPath(arguments[i]);
                params.inputBasename = basename(arguments[i]);

                // Create default output in case none is specified
                params.outputBasename = params.inputBasename + L"_booklet";
                params.outputPath = precedingPathWithoutFilename(params.inputFullPath);
                params.outputType = eFFPDF;
                params.outputFullPath = params.outputPath +
                    params.outputBasename +
                    extensionFromFormat(params.outputType);
            }
            else
            {
                params.outputPath = precedingPathWithoutFilename(arguments[i]);
                params.outputBasename = basename(arguments[i]) + L"_booklet";
                params.outputType = fileFormatFromPath(arguments[i]);
                params.outputFullPath = params.outputPath +
                    params.outputBasename +
                    extensionFromFormat(params.outputType);
            }
        }
        else
        {
            // An argument
            String setting = arguments[i].substr(0, equalsPos);
            std::transform(setting.begin(), setting.end(), setting.begin(), towlower);
            String value = arguments[i].substr(equalsPos + 1);
            try {
                if (setting == L"pw")
                {
                    params.userPassword = StringToU8String(value);
                }
                else if (setting == L"f")
                {
                    transform(value.begin(), value.end(), value.begin(), towlower);
                    if (value == L"yes" || value == L"true")
                        params.flattenTransparency = true;
                }
                else if (setting == L"o")
                {
                    transform(value.begin(), value.end(), value.begin(), towlower);
                    if (value == L"yes" || value == L"true")
                        params.simulateOverprint = true;
                }
                else if (setting == L"s")
                {
                    transform(value.begin(), value.end(), value.begin(), towlower);
                    if (value == L"yes" || value == L"true")
                        params.sequential = true;
                }
                else if (setting == L"p")
                {
                    transform(value.begin(), value.end(), value.begin(), towupper);
                    if (pageSizes.find(value) != pageSizes.end())
                    {
                        params.spreadWidth = pageSizes[value].width;
                        params.spreadHeight = pageSizes[value].height;
                    }
                }
            }
            catch (std::exception)
            {
                String message(L"Invalid value: ");
                message += setting + L"=" + value;
                throw std::invalid_argument(StringToU8String(message).c_str());
            }
        }
    }

    return params;
}

static bool dropOverprintForCMYKBlackText(void *val, const IDOMNodePtr &node)
{
    try {
        //IJawsMako* mako = (IJawsMako*)val;

        const IDOMGlyphsPtr glyphs = edlobj2IDOMGlyphs(node);
        if (!glyphs)
        {
            // Don't care
            return true;
        }

        const IDOMBrushPtr brush = glyphs->getFill();
        const IDOMSolidColorBrushPtr solid = edlobj2IDOMSolidColorBrush(brush);
        if (!solid)
        {
            // Don't care
            return true;
        }

        // Grab the colour and space
        IDOMColorPtr colour = solid->getColor();
        IDOMColorSpacePtr space = colour->getColorSpace();

        // Look for DeviceCMYK
        if (space->getColorSpaceType() != IDOMColorSpace::eDeviceCMYK)
        {
            // Don't care
            return true;
        }

        // Grab the colourants
        const float c = colour->getComponentValue(0);
        const float m = colour->getComponentValue(1);
        const float y = colour->getComponentValue(2);
        const float k = colour->getComponentValue(3);

        if (c != 0.0f ||
            m != 0.0f ||
            y != 0.0f ||
            k != 1.0f)
        {
            // Don't care
            return true;
        }

        // Simply remove overprint. We can do this by clearing the device parameter
        // properties. The only parameter that belongs to glyphs is overprint, so
        // this is safe.
        node->removeProperty("DeviceParams");
    }
    catch (IEDLError &e)
    {
        throwEDLError(e.getErrorCode());
    }
    return true;
}

// Create a IDOMFixedPage from an IPage, rotating content and cropbox as needed
static bool applyPageRotation(IJawsMakoPtr jawsMako, IPagePtr page, IDOMFixedPagePtr& fixedPage, FRect& cropBox)
{
    if (!page)
        return false;

    // Does the page have crop margins?
    // Note: page->getCropBox() will return the cropbox dimensions if set, or the mediabox (absolute page size) dimensions if not
    // A fixedPage() does not offer this guarantee, so fixedPage->getCropBox() may return an empty FRect() (ie one or more values < 0)
    cropBox = page->getCropBox();

    // Is the page rotated?
    const int32 rotationDegrees = (page->getRotate() + 360) % 360;

    // Create a fixed page from the page contents, editable if content is to be rotated
    fixedPage = rotationDegrees ? page->clone()->edit() : page->getContent();

    // Rotate content as required
    if (rotationDegrees)
    {
        const double width = fixedPage->getWidth();
        const double height = fixedPage->getHeight();

        FMatrix rotate;

        switch (rotationDegrees / 90)
        {
        case 1: // 90 degrees
            fixedPage->setWidth(height);
            fixedPage->setHeight(width);
            rotate.setDX(height);
            break;

        case 2: // 180 degrees
            rotate.setDX(width);
            rotate.setDY(height);
            break;

        case 3: // 270 degrees
            fixedPage->setWidth(height);
            fixedPage->setHeight(width);
            rotate.setDY(width);
            break;

        default:
            break;
        }

        rotate.rotate(rotationDegrees * PI / 180.0);

        // Extract page objects into a group with the transform matrix at its root 
        IDOMGroupPtr transformGroup = IDOMGroup::create(jawsMako, rotate);
        IDOMNodePtr node;
        while ((node = fixedPage->extractChild(IDOMNodePtr())) != nullptr)
            transformGroup->appendChild(node);

        // Add rotated objects to page
        fixedPage->appendChild(transformGroup);

        // Rotate cropbox
        rotate.transformRect(cropBox);
    }
    fixedPage->setCropBox(cropBox);

    return true;
}


// Impose an individual page on the spread
void imposePage(const IJawsMakoPtr &jawsMako, const IDOMFixedPagePtr &spread, const IPagePtr &page, bool isLeftPage)
{
    // There may not be a page
    if (!page)
    {
        return;
    }

    // Otherwise grab the fixed page content from the source page
    //IDOMFixedPagePtr content = page->getContent();

    IDOMFixedPagePtr content;
    FRect cropBox;
	applyPageRotation(jawsMako, page, content, cropBox);

    // Work out how to transform the contents of the page to the position we
    // want in the spread.
    FMatrix transform;

    const double targetWidth = spread->getWidth() / 2.0;
    const double targetHeight = spread->getHeight();

    double sourceWidth = content->getWidth();
    double sourceHeight = content->getHeight();
    const double originalWidth = sourceWidth;
    const double originalHeight = sourceHeight;

	//// First, Rotate counter clockwise if the source page is landscape
    //if (sourceWidth > sourceHeight)
    //{
    //    // Rotate counter clockwise
    //    transform = FMatrix(0.0, -1.0, 1.0, 0.0, 0.0, sourceWidth);
    //    double tmp = sourceWidth;
    //    sourceWidth = sourceHeight;
    //    sourceHeight = tmp;
    //}

    // Next determine how much we need to scale that to fit.
    double scaleX = targetWidth / sourceWidth;
    double scaleY = targetHeight / sourceHeight;
    const double scale = scaleX > scaleY ? scaleY : scaleX;

    // Apply and center
    const double scaledWidth = sourceWidth * scale;
    const double scaledHeight = sourceHeight * scale;
    const FMatrix scaleMatrix(scale, 0.0, 0.0, scale, (targetWidth - scaledWidth) / 2.0, (targetHeight - scaledHeight) / 2.0);
    transform.postMul(scaleMatrix);

    // Lastly, if the page is on the right, then we shunt everything across accordingly
    if (!isLeftPage)
    {
        transform.setDX(transform.dx() + targetWidth);
    }

    // Make a group with that transform, and clip to the page area
    const IDOMGroupPtr transformGroup = IDOMGroup::create(jawsMako, transform, IDOMPathGeometry::create(jawsMako, FRect(0.0, 0.0, originalWidth, originalHeight)));

// Copy in all the source DOM into that group
IDOMNodePtr child = content->getFirstChild();
while (child)
{
    child->cloneTreeAndAppend(jawsMako, transformGroup);
    child = child->getNextSibling();
}

    // Append the group to the spread
    spread->appendChild(transformGroup);

    // We can release the content; we will not use it again
    page->release();

    // Done
}

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
{
    _setmode(_fileno(stderr), _O_U16TEXT);
#else
int main(int argc, char *argv[])
{
#endif

    try
    {
        // Get page sizes
        std::map<String, sPageSize> PageSizes = GetPageSizeList();
        
        // Check number of arguments
        if (argc < 2)
        {
            usage(PageSizes);
            return 1;
        }

        // Copy command line parameters to a Mako String array
        CEDLStringVect argString;
        for (int i = 1; i < argc; i++)
        {
#ifdef _WIN32
            argString.append(argv[i]);
#else
            argString.append(U8StringToString(U8String(argv[i])));
#endif
        }

        const sParameters params = parse_params(argString, PageSizes);

        // Create our JawsMako instance
        const IJawsMakoPtr jawsMako = IJawsMako::create();
        IJawsMako::enableAllFeatures(jawsMako);

        // Timer
        const clock_t begin = clock();

        // Create our inputs and outputs
        IInputPtr  input  = IInput::create(jawsMako, params.inputType);
        if (params.userPassword.size())
        {
            IPDFInputPtr pdfInput = obj2IPDFInput(input);

            pdfInput->setPassword(params.userPassword);
        }
        IOutputPtr output = IOutput::create(jawsMako, params.outputType);
        //if (params.flattenTransparency)
        //    output->setVersion(IPDFOutput::ePDFX1a);

        // Make XPS output RGB (like MakoConverter)
        IXPSOutputPtr xpsOutput = obj2IXPSOutput(output);
        if (xpsOutput)
        {
            xpsOutput->setTargetColorSpace(IDOMColorSpacesRGB::create(jawsMako));
        }

        // Get the document from the input; there is only one document in the file for PDF
        IDocumentPtr sourceDocument = input->open(params.inputFullPath)->getDocument();
        
        // Create an assembly and document for our output
        IDocumentAssemblyPtr assembly = IDocumentAssembly::create(jawsMako);
        IDocumentPtr document = IDocument::create(jawsMako);
        assembly->appendDocument(document);

        // Create an output writer to a hard coded file name in the current directory
        //IOutputWriterPtr outputWriter = output->openWriter(assembly, params.outputPath);

        // Begin writing our empty document
        //outputWriter->beginDocument(document);

        // Create the overprint simulation transform
        IOverprintSimulationTransformPtr transform = IOverprintSimulationTransform::create(jawsMako);
        transform->setSimulateBlackDeviceGrayTextOverprint(false);
        transform->setResolution(600);

        // Setup a renderer transform to perform transparency flattening.
        IRendererTransformPtr renderer = IRendererTransform::create(jawsMako);
        renderer->setTargetSpace(IDOMColorSpaceDeviceCMYK::create(jawsMako)); // Same colorspace as overprint transform
        renderer->renderTransparentNodes(true); // Render transparent nodes
        renderer->setResolution(600);

        // We're creating a booklet on landscape pages that when printed duplex, folded and stapled
        // will result in a booklet. All pages will be scaled to fit on an the target size and centered
        // as required.

        // We'll use the chosen size of spread in landscape, scaling individual pages to fit.
        double spreadWidth = params.spreadHeight;
        double spreadHeight = params.spreadWidth;
        
        // If no page size was specified, determine a spread size from the first page 
    	if (spreadWidth == 0)
        {
            spreadWidth = sourceDocument->getPage(0)->getWidth() * 2.0;
            spreadHeight = sourceDocument->getPage(0)->getHeight();
        }

        // Switch the dimensions around if the page happened to be longer than high, eg an envelope
        if (spreadHeight > spreadWidth)
        {
            spreadWidth = params.spreadWidth;
            spreadHeight = params.spreadHeight;
        }

        // How many spreads will there be?
        uint32 numSpreads;
        if (params.sequential)
            numSpreads = (sourceDocument->getNumPages() + 1) / 2;
        else
    		numSpreads = (sourceDocument->getNumPages() + 3) / 4 * 2;

        // So, for each spread
        for (uint32 i = 0; i < numSpreads; i++)
        {
            // Pull the two pages required for the spread from the source document.
            // Note that one or more of the sides may be blank
            IPagePtr pageA;
            uint32 pageANum;
            if (params.sequential)
                pageANum = i * 2;
            else
                pageANum = i;

            if (pageANum < sourceDocument->getNumPages())
            {
                pageA = sourceDocument->getPage(pageANum);
            }

            IPagePtr pageB;
            uint32 pageBNum;
            if (params.sequential)
                pageBNum = (i * 2) + 1;
            else
                pageBNum = (numSpreads * 2) - i - 1;

            if (pageBNum < sourceDocument->getNumPages())
            {
                pageB = sourceDocument->getPage(pageBNum);
            }

            // If we're on an even spread (of a booklet), then pageA belongs on the right
            if (!params.sequential && i % 2 == 0)
            {
                // Swap
                IPagePtr tmp = pageA;
                pageA = pageB;
                pageB = tmp;
            }

            // Simulate overprint if required (transform the source pages)
            if (params.simulateOverprint)
            {
                std::wcout << L"Simulating overprint on spread " << i << L"..." << std::endl;
                // Drop overprint for DeviceCMYK text
                if (pageA) {
                    pageA->edit()->walkTree(dropOverprintForCMYKBlackText, jawsMako, true, true);
                    transform->transformPage(pageA);
                }
                if (pageB) {
                    pageB->edit()->walkTree(dropOverprintForCMYKBlackText, jawsMako, true, true);
                    transform->transformPage(pageB);
                }
            }

            // Create a new fixed page for the spread. Units are 1/96th of an inch
            IDOMFixedPagePtr spread = IDOMFixedPage::create(jawsMako, spreadWidth, spreadHeight);

            IPagePtr outputPage = sourceDocument->getPage(0);

            // Copy in the data for the left page
            imposePage(jawsMako, spread, pageA, true);

            // And the right
            imposePage(jawsMako, spread, pageB, false);

            // Flatten transparency if required
            if (params.flattenTransparency) {
                std::wcout << L"Flattening spread " << i << L"..." << std::endl;
                bool changed;
                spread = edlobj2IDOMFixedPage(renderer->transform(spread, changed));
                if (!spread)
                {
                    // This should never happen in practice.
                    throw std::runtime_error("Result of transparency flattening is null or not a page!?");
                }
            }

            // Wrap in an IPage and write to the output
            IPagePtr page = IPage::create(jawsMako);
            page->setContent(spread);
            // outputWriter->writePage(page);
            document->appendPage(page);
        }

        // Finish up writing
        //outputWriter->endDocument();
        //outputWriter->finish();
        std::wcout << L"Writing \'";
        std::wcerr << params.outputFullPath;
        std::wcout << L"\'..." << std::endl;
        output->writeAssembly(assembly, params.outputFullPath);

        clock_t end = clock();
        double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        std::wcout << L"Elapsed time: " << elapsed_secs << L" seconds." << std::endl;
        // Done!
    }
    catch (IError &e)
    {
        String errorFormatString = getEDLErrorString(e.getErrorCode());
        std::wcerr << L"Exception thrown: " << e.getErrorDescription(errorFormatString) << std::endl;
        return e.getErrorCode();
    }
    catch (std::exception &e)
    {
        std::wcerr << L"std::exception thrown: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}