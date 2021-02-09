// -----------------------------------------------------------------------
//  <copyright file="makowatermarker.cpp" company="Global Graphics Software Ltd">
//      Copyright (c) 2021 Global Graphics Software Ltd. All rights reserved.
//  </copyright>
//  <summary>
//  This example is provided on an "as is" basis and without warranty of any kind.
//  Global Graphics Software Ltd. does not warrant or make any representations regarding the use or
//  results of use of this example.
//  </summary>
// -----------------------------------------------------------------------

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <wctype.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <jawsmako/jawsmako.h>
#include <jawsmako/pdfinput.h>
#include <jawsmako/pdfoutput.h>
#include <edl/idomglyphs.h>
#include <edl/edlnamespaces.h>
#include <math.h>
#include <jawsmako/xpsoutput.h>

#ifdef _WIN32
#include <fcntl.h>
#include <corecrt_io.h>
#include <direct.h>
typedef int mode_t;
#endif

using namespace JawsMako;
using namespace EDL;

struct parameters
{
    String inputFullPath;
    String inputBasename;
    eFileFormat inputType;
    U8String userPassword;
    String outputPath;
    String outputBasename;
    eFileFormat outputType;
    String watermarkText;
    String watermarkPdf;
    int angle;
    bool useIncrementalOutput;
    int redValue;
    int blueValue;
    int greenValue;
    int opacityValue;
    U8String fontName;
};

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

    if (extension == L".pdf")
        return eFFPDF;
    if (extension == L".xps")
        return eFFXPS;
    if (extension == L".ps" || extension == L".eps")
        return eFFPS;
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

// Return filename without preceding path
static String filenameWithoutPrecedingPath(const String &path)
{
    std::string filepath = StringToU8String(path).c_str();
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
    const size_t extensionPosition = filepath.find_last_of('.');
    if (extensionPosition != String::npos)
        return U8StringToString(filepath.substr(0, extensionPosition).c_str());
    return path;
}

static void usage()
{
    std::wcout << "Mako Watermarker v1.2.0\n" << std::endl;
    std::wcout << L"Usage:" << std::endl;
    std::wcout << L"   MakoWatermarker <source.pdf> [<output.pdf>] [parameter=setting] [parameter=setting] ..." << std::endl;
    std::wcout << L"\nWhere:" << std::endl;
    std::wcout << L"   input.pdf          source PDF" << std::endl;
    std::wcout << L"   output.pdf         PDF file to write the output to." << std::endl;
    std::wcout << L"                        If no output file is specified, <source file>_watermarked.pdf is assumed." << std::endl;
    std::wcout << L"   parameter=setting  one or more settings, described below." << std::endl;
    std::wcout << L"\nParameters:" << std::endl;
    std::wcout << L"   t=<watermark>      Text of watermark, eg 'Draft'. Surround with quotes if the text contains spaces" << std::endl;
    std::wcout << L"   f=<font name>      Font to use, eg 'Yu Gothic Bold'. Surround with quotes if the name contains spaces" << std::endl;
    std::wcout << L"   w=<watermark pdf>  Use first page of the specified PDF as the watermark content." << std::endl;
    std::wcout << L"   a=<angle>          Angle from -180" << char(176) << "(anti-clockwise) to +180" << char(176) << "(clockwise) of rotation" << std::endl;
    std::wcout << L"                        If no angle is specified, a default of zero (ie horizontal) is assumed)" << std::endl;
    std::wcout << L" The next four parameters control the color and opacity of the watermark text" << std::endl;
    std::wcout << L"   r=<red>            Red component % value in range 0 - 100. Default is zero" << std::endl;
    std::wcout << L"   g=<green>          Green component % value in range 0 - 100. Default is 80%" << std::endl;
    std::wcout << L"   b=<blue>           Blue component % value in range 0 - 100. Default is 80%" << std::endl;
    std::wcout << L"   o=<opacity>        Opacity % value in range 0 - 100. Default is 40%" << std::endl << std::endl;
    std::wcout << L"   i=<yes|no>         Incremental save:" << std::endl;
    std::wcout << L"                        Y = use it (default)" << std::endl;
    std::wcout << L"                        N = do not use it; processing will take longer but may produce smaller output\n" << std::endl;
}

int zeroToOneHundred(int i)
{
    if (i < 0)
        i = 0;
    if (i > 100)
        i = 100;
    return i;
}

static parameters parse_params(CEDLStringVect arguments)
{
    parameters params;

    // Set defaults
    params.watermarkText = L"My Favorite Test";
    params.angle = 0;
    params.redValue = 0;
    params.greenValue = 80;
    params.blueValue = 80;
    params.opacityValue = 40;
    params.useIncrementalOutput = true;
    params.fontName = "Arial Bold";

    for (uint8 i = 0; i < arguments.size(); i++)
    {
        auto testArg = arguments[i];
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
                params.outputBasename = params.inputBasename + L"_watermarked";
                params.outputPath = precedingPathWithoutFilename(params.inputFullPath);
                params.outputType = eFFPDF;
            }
            else
            {
                if (arguments[i] != params.inputFullPath) // Set the output only if it is different
                {
                    params.outputPath = precedingPathWithoutFilename(arguments[i]);
                    params.outputBasename = basename(arguments[i]);
                    params.outputType = fileFormatFromPath(arguments[i]);
                }
            }
        }
        else
        {
            // An argument
            String setting = arguments[i].substr(0, equalsPos);
            std::transform(setting.begin(), setting.end(), setting.begin(), towlower);
            String value = arguments[i].substr(equalsPos + 1);
            try {
                if (setting == L"t")
                {
                    params.watermarkText = value;
                }
                if (setting == L"f")
                {
                    params.fontName = StringToU8String(value);
                }
                if (setting == L"w")
                {
                    params.watermarkPdf = value;
                }
                if (setting == L"i")
                {
                    std::transform(value.begin(), value.end(), value.begin(), towlower);
                    params.useIncrementalOutput = 
                        wcscmp(value.c_str(), L"yes") || 
                        wcscmp(value.c_str(), L"true") || 
                        wcscmp(value.c_str(), L"y") || 
                        wcscmp(value.c_str(), L"t");
                }
                else if (setting == L"r")
                {
                    wchar_t* end;
                    params.redValue = zeroToOneHundred(abs(std::wcstol(value.c_str(), &end, 10)));
                }
                else if (setting == L"g")
                {
                    wchar_t* end;
                    params.greenValue = zeroToOneHundred(abs(std::wcstol(value.c_str(), &end, 10)));
                }
                else if (setting == L"b")
                {
                    wchar_t* end;
                    params.blueValue = zeroToOneHundred(abs(std::wcstol(value.c_str(), &end, 10)));
                }
                else if (setting == L"o")
                {
                    wchar_t* end;
                    params.opacityValue = zeroToOneHundred(abs(std::wcstol(value.c_str(), &end, 10)));
                }
                else if (setting == L"a")
                {
                    wchar_t* end;
                    int a = abs(std::wcstol(value.c_str(), &end, 10));
                    if (a < -180)
                        a = -180;
                    if (a > 180)
                        a = 180;
                    params.angle = a;
                }
            }
                // ReSharper disable once CppDeclaratorNeverUsed
            catch (std::exception &e)
            {
                String message(L"Invalid value: ");
                message += setting + L"=" + value;
                throw std::invalid_argument(StringToU8String(message).c_str());
            }
        }
    }
    return params;
}

// Create watermark DOM from text
static IDOMGroupPtr WatermarkFromText(IJawsMakoPtr jawsMako, parameters params)
{
    // Choose a font. Default to Arial Bold as it's likely to be present.
    uint32 fontIndex; // In case the font is inside a TrueType collection
    IDOMFontPtr font;
    try {
        font = jawsMako->findFont(params.fontName, fontIndex);
    }
    catch(IEDLError &e)
    {
        if (e.getErrorCode() == JM_ERR_FONT_NOT_FOUND)
            font = jawsMako->findFont("Arial Bold", fontIndex);
    }

    // A brush for the watermark.
    const IDOMBrushPtr solidBrush = IDOMSolidColorBrush::create(jawsMako,
        IDOMColor::create(jawsMako, IDOMColorSpacesRGB::create(jawsMako),
            1.0,
            float(params.redValue / 100.0f),
            float(params.greenValue / 100.0f),
            float(params.blueValue / 100.0f)
        ));

    // A transform to rotate the text by the specified angle of rotation
    FMatrix rotate = FMatrix();
    rotate.rotate(double(params.angle) * (PI / 180.0));

    // Create the glyphs.
    IDOMGlyphsPtr glyphs = IDOMGlyphs::create(jawsMako, params.watermarkText, 120, FPoint(0.0, 0.0), solidBrush,
        font, fontIndex, IDOMGlyphs::eSSNone, rotate);
    const FRect glyphBounds = glyphs->getBounds();
    IDOMGroupPtr group = IDOMGroup::create(jawsMako, FMatrix(), IDOMPathGeometry::create(jawsMako, glyphBounds));
    group->appendChild(glyphs);
    
    return group;
}

// Create watermark DOM from a file
static IDOMGroupPtr WatermarkFromFile(IJawsMakoPtr jawsMako, parameters params, FMatrix &rotate)
{
    if (fileExists(params.watermarkPdf)) {
        // Create a PDF input
        IPDFInputPtr input = IPDFInput::create(jawsMako);

        // Get the page from the input
        IPagePtr page = input->open(params.watermarkPdf)->getDocument()->getPage(0);
        const FRect cropBox = page->getCropBox();

        // and content
        const IDOMFixedPagePtr pageContent = page->getContent();

        // Release page, we no longer need it
        page->release();

        // A transform to rotate the content by the specified angle of rotation
        rotate.rotate(double(params.angle) * (PI / 180.0));

        // Make a group with that transform
        IDOMGroupPtr group = IDOMGroup::create(jawsMako, rotate, IDOMPathGeometry::create(jawsMako, cropBox));

        // Copy all the source DOM into that group
        IDOMNodePtr child = pageContent->getFirstChild();
        while (child)
        {
            child->cloneTreeAndAppend(jawsMako, group);
            child = child->getNextSibling();
        }

        return group;
    }
    params.watermarkText = L"Watermark PDF not found";
    return WatermarkFromText(jawsMako, params);
}

// Create a watermark
static IDOMFormPtr CreateWatermark(IJawsMakoPtr jawsMako, IPagePtr &page, parameters params)
{
    FMatrix adjuster = FMatrix();
    IDOMGroupPtr transformGroup;
    if (params.watermarkPdf.size())
        transformGroup = WatermarkFromFile(jawsMako, params, adjuster);
    else
        transformGroup = WatermarkFromText(jawsMako, params);

    FRect contentBounds = transformGroup->getBounds();
    
    const double pageWidth = page->getWidth();
    const double pageHeight = page->getHeight();

    // Scale to fill the page, with a 5% margin
    double scale;
    if (contentBounds.dX > contentBounds.dY)
        scale = pageWidth * 0.95 / contentBounds.dX;
    else
        scale = pageHeight * 0.95 / contentBounds.dY;
    adjuster.scale(scale, scale);
    transformGroup->setRenderTransform(adjuster);
    contentBounds = transformGroup->getBounds();
    
    // We want to move the glyphs to here
    const FPoint position((pageWidth - contentBounds.dX) / 2.0,
                    (pageHeight - contentBounds.dY) / 2.0);
    
    // So adjust the rotation matrix to move it here
    adjuster.setDX(position.x - contentBounds.x + adjuster.dx());
    adjuster.setDY(position.y - contentBounds.y + adjuster.dy());
    transformGroup->setRenderTransform(adjuster);
    contentBounds = transformGroup->getBounds();

    IDOMFormPtr xform = IDOMForm::create(jawsMako, FMatrix(), contentBounds);

    //// Draw box around watermark for debug purposes
    //IDOMPathNodePtr path = IDOMPathNode::createStroked(jawsMako, IDOMPathGeometry::create(jawsMako, contentBounds),
    //                                                   IDOMSolidColorBrush::create(
    //                                                       jawsMako, IDOMColor::create(
    //                                                           jawsMako, IDOMColorSpacesRGB::create(jawsMako), 1.0f,
    //                                                           0.0f, 1.0f, 0.0f)));
    //path->setStrokeThickness(4);
    //xform->appendChild(path);

    xform->appendChild(transformGroup);
    return xform;
}

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);
#else
int main(int argc, char *argv[])
{
#endif

    try
    {
        // Check number of arguments
        if (argc < 2)
        {
            usage();
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

        static const wchar_t *timeFormat = L"%c %Z";
        std::time_t t = std::time(nullptr);
        std::tm timeStamp = *std::gmtime(&t);
        std::wcout << L"Start:            " << std::put_time(&timeStamp, timeFormat) << std::endl;
        const parameters params = parse_params(argString);

        // Create our JawsMako instance.
        const IJawsMakoPtr jawsMako = IJawsMako::create();
        IJawsMako::enableAllFeatures(jawsMako);
        
        // Create input
        IInputPtr input  = IInput::create(jawsMako, params.inputType);

        // Get the assembly from the input.
        IDocumentAssemblyPtr assembly = input->open(params.inputFullPath);
        IDocumentPtr document = assembly->getDocument();
        const uint32 pageCount = document->getNumPages();
        IPagePtr page = document->getPage(0);

        // Create watermark in a PDF form
        IDOMFormPtr watermark = CreateWatermark(jawsMako, page, params);

        // Apply the watermark to every page
        for (uint32 pageNum = 0; pageNum < pageCount; pageNum++)
        {
            // A FormInstance is needed to hold the form (one per page)
            IDOMFormInstancePtr formInstance = createInstance<IDOMFormInstance>(jawsMako, CClassID(IDOMFormInstanceClassID));
            formInstance->setOpacity(float(params.opacityValue / 100.0f));
            formInstance->setForm(watermark);
            page = document->getPage(pageNum);
            IDOMFixedPagePtr fixedPage = page->edit();
            fixedPage->appendChild(formInstance);
        }
    
        // Now we can write this out
        t = std::time(nullptr);
        timeStamp = *std::gmtime(&t);

        const String outputFullPath = params.outputPath + params.outputBasename + extensionFromFormat(params.outputType);
        std::wcout << L"Writing:          \'";
        std::wcerr << outputFullPath;
        std::wcout << L"\'...";
        std::wcerr << std::endl;
        IOutputPtr output = IOutput::create(jawsMako, params.outputType);
        
        // Set incremental output
        if(params.outputType == eFFPDF)
        {
            IPDFOutputPtr pdfOutput = obj2IPDFOutput(output);
            if (pdfOutput)
                pdfOutput->setEnableIncrementalOutput(params.useIncrementalOutput);
        }

        // Make XPS output RGB
        IXPSOutputPtr xpsOutput = obj2IXPSOutput(output);
        if (xpsOutput)
        {
            xpsOutput->setTargetColorSpace(IDOMColorSpacesRGB::create(jawsMako));
        }

         output->writeAssembly(assembly, outputFullPath);

        t = std::time(nullptr);
        timeStamp = *std::gmtime(&t);
        std::wcout << L"Done:             " << std::put_time(&timeStamp, timeFormat) << std::endl;

        return 0;
    }
    catch (IError &e)
    {
        const String errorFormatString = getEDLErrorString(e.getErrorCode());
        std::wcerr << L"Exception thrown: " << e.getErrorDescription(errorFormatString) << std::endl;
        return e.getErrorCode();
    }
    catch (std::exception &e)
    {
        std::wcerr << L"std::exception thrown: " << e.what() << std::endl;
        return 1;
    }
}
