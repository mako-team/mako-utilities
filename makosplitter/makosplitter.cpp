// -----------------------------------------------------------------------
//  <copyright file="makosplitter.cpp" company="Global Graphics Software Ltd">
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
#include <iostream>
#include <stdexcept>
#include <jawsmako/jawsmako.h>
#include <jawsmako/pdfoutput.h>
#include <thread>
#include <mutex>
#include <vector>
#include <jawsmako/xpsoutput.h>
#include <jawsmako/pdfinput.h>

#ifdef _WIN32
#include <fcntl.h>
#include <corecrt_io.h>
#include <direct.h>
typedef int mode_t;
#endif

#if defined(_WIN32)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__GNUC__)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#elif defined(__APPLE__)
#include <filesystem>
namespace fs = std::__fs::filesystem;
#endif

using std::string;
using std::thread;
using std::mutex;
using std::vector;

using namespace JawsMako;
using namespace EDL;

struct sJob
{
    uint32 chunkSize;
    IDocumentPtr sourceDocument;
    CEDLVector<IPagePtr> clonedPages;
    eFileFormat outputType;
    String outputFile;
    bool deepCopy;
};

struct sParameters
{
    String inputFullPath;
    String inputBasename;
    eFileFormat inputType;
    U8String userPassword;
    String outputPath;
    String outputBasename;
    eFileFormat outputType;
    uint32 chunkSize;
    bool singleThread;
    bool deepCopy;
};

// Globals
static mutex globalMtx;
static bool makoDemoReporting;
static String pathSeparator;


static void usage()
{
    std::wcout << "Mako Splitter v1.2.0\n" << std::endl;
    std::wcout << L"   Makosplitter input.xxx [output.yyy] [parameter=setting] [parameter=setting] ..." << std::endl;
    std::wcout << L" Where:" << std::endl;
    std::wcout << L"   input.xxx          source file from which to extract pages, where xxx is pdf, xps, pxl (PCL/XL) or pcl (PCL5)." << std::endl;
    std::wcout << L"   output.yyy         target file to write the output to, where yyy is pdf, xps, pxl or pcl." << std::endl;
    std::wcout << L"                        If no output file is declared, <input>.pdf is assumed." << std::endl;
    std::wcout << L"   parameter=setting  one or more settings, described below." << std::endl;
    std::wcout << std::endl;
    std::wcout << L"Parameters:" << std::endl;
    std::wcout << L"   pw=<password>      PDF password, if required to open the file." << std::endl;
    std::wcout << L"   c=<chunk size>     The number of pages per output file (omitted or 0 means one file per page)" << std::endl;
    std::wcout << L"   f=yes|no           Create a folder to contain the output, named according to the output file name. Default is no folder." << std::endl;
    std::wcout << L"   s=yes|no           Use a single thread (yes), otherwise multiple threads are used to write the output files, the default." << std::endl;
    std::wcout << L"   d=yes|no           Use a deep copy of pages, ie copy bookmarks and form field metadata. May negatively impact performance." << std::endl;
    std::wcout << L"                        Default is no." << std::endl;
    //std::wcout << L"   z=on|off           Hidden option: Report filename as it is processed on STDERR (to support MakoDemo)" << std::endl;
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
    if (extension == L".ps")
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
    const std::string filepath = StringToU8String(path).c_str();
    const size_t lastPathSeparator = filepath.find_last_of(PATH_SEP_CHAR);
    if (lastPathSeparator != String::npos)
        return U8StringToString(filepath.substr(lastPathSeparator + 1).c_str());
    return U8StringToString(filepath.c_str());
}

// Return preceding path without filename
static String precedingPathWithoutFilename(const String &path)
{
    const std::string filepath = StringToU8String(path).c_str();
    const size_t lastPathSeparator = filepath.find_last_of(PATH_SEP_CHAR);
    if (lastPathSeparator != String::npos)
        return U8StringToString(filepath.substr(0, lastPathSeparator + 1).c_str());
    return L"." + pathSeparator; // assume current folder
}

// Return basename only 
static String basename(const String &path)
{
    const std::string filepath = StringToU8String(filenameWithoutPrecedingPath(path)).c_str();
    const size_t extensionPosition = filepath.find_last_of('.');
    if (extensionPosition != String::npos)
        return U8StringToString(filepath.substr(0, extensionPosition).c_str());
    return path;
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

// Populate params structure with items specified on the command line
static sParameters parseParams(CEDLStringVect arguments)
{
    std::string p(1, PATH_SEP_CHAR);
    String pathSeparator = U8StringToString(p.c_str());

    sParameters params;

    // Set defaults
    params.userPassword = "";
    params.inputType = eFFPDF;
    params.outputType = eFFPDF;
    params.chunkSize = 1;
    params.singleThread = false;
    makoDemoReporting = false;

    for (uint32 i = 0; i < arguments.size(); i++)
    {
        auto testarg = arguments[i];
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
                params.outputBasename = params.inputBasename;
                params.outputPath = precedingPathWithoutFilename(params.inputFullPath);
            }
            else
            {
                params.outputPath = precedingPathWithoutFilename(arguments[i]);
                params.outputBasename = basename(arguments[i]);
                params.outputType = fileFormatFromPath(arguments[i]);
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
                else if (setting == L"c")
                {
                    wchar_t* end;
                    params.chunkSize = abs(std::wcstol(value.c_str(), &end, 10));
                }
                else if (setting == L"f")
                {
                    transform(value.begin(), value.end(), value.begin(), towlower);
                    if (value == L"yes" || value == L"true")
                        params.outputPath += pathSeparator + params.outputBasename;
                }
                else if (setting == L"s")
                {
                    transform(value.begin(), value.end(), value.begin(), towlower);
                    params.singleThread = value == L"yes" || value == L"true";
                }
                else if (setting == L"d")
                {
                    transform(value.begin(), value.end(), value.begin(), towlower);
                    params.deepCopy = value == L"yes" || value == L"true";
                }
                else if (setting == L"z")
                {
                    transform(value.begin(), value.end(), value.begin(), towlower);
                    makoDemoReporting = value == L"yes" || value == L"true";
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

// Append one or more pages to a new assembly and document, then output as a new file
static void writeChunk(IJawsMakoPtr& mako, uint32 chunkSize, IDocumentPtr sourceDocument, bool deepCopy, CEDLVector<IPagePtr> clonedPages, String& outputFile, IOutputPtr& output)
{
IDocumentAssemblyPtr assembly = IDocumentAssembly::create(mako);
IDocumentPtr document = IDocument::create(mako);
for (uint32 i = 0; i < chunkSize; i++)
{
    if (!deepCopy)
        document->appendPage(clonedPages[i]);
    else
        document->appendPage(clonedPages[i], sourceDocument);
}
assembly->appendDocument(document);
output->writeAssembly(assembly, outputFile);
    if (makoDemoReporting)
    {
        globalMtx.lock();
        std::wcerr << outputFile << std::endl;
        globalMtx.unlock();
    }
}

// Run the job
static void threadRunner(IJawsMakoPtr mako, vector<sJob>* jobs)
{
    IOutputPtr output = IOutput::create(mako, (*jobs)[0].outputType);
    // Make XPS output RGB (like regular MakoConverter)
    IXPSOutputPtr xpsOutput = obj2IXPSOutput(output);
    if (xpsOutput)
        xpsOutput->setTargetColorSpace(IDOMColorSpacesRGB::create(mako));
    const size_t count = jobs->size();
    for (size_t i = 0; i < count; ++i) {
        sJob job = (*jobs)[i];
        writeChunk(mako, job.chunkSize, job.sourceDocument, job.deepCopy, job.clonedPages, job.outputFile, output);
    }
}

// Return the page range to be added to the output filename
static std::wstring pageIndex(const uint32 pageFrom, const uint32 pageCount)
{
    if (pageCount == 1)
    {
        return std::wstring(
            std::wstring(L"_p") +
            std::to_wstring(pageFrom)
        );
    }
    return std::wstring (
        std::wstring(L"_p") +
        std::to_wstring(pageFrom) +
        std::wstring(L"-") +
        std::to_wstring(pageFrom + pageCount - 1)
    );
}

// Divide the PDF into chunks of the required size and set up job(s) to output the corresponding range of pages
// Then run the jobs on the available threads
void dumpChunks(IJawsMakoPtr mako, IDocumentPtr document, uint32 pageCount, uint32 chunkSize, String folder, String _outputFile, eFileFormat outputType, bool runSingleThreaded, bool deepCopy)
{
    const uint32 chunkCount = pageCount / chunkSize;
    const uint32 finalChunkSize = pageCount % chunkSize;

    // How many threads are there available?
    unsigned int availableWorkers = thread::hardware_concurrency();

    // Adjust number of available workers if they are not required
    if (chunkCount == 0 || runSingleThreaded)
        availableWorkers = 1;
    else
        if (chunkCount < availableWorkers)
            availableWorkers = chunkCount;

    unsigned int threadCount = availableWorkers - 1;
    thread* workers = new thread[threadCount];

    // Create an array to hold the jobs
    vector<sJob>* jobs = new vector<sJob>[availableWorkers];

    // Append a trailing separator
    std::basic_string<wchar_t> pathSep(1, PATH_SEP_CHAR);
    std::wstring folderPath(folder.c_str());
    auto lastChar = folderPath.substr(folderPath.length() - 1);
    if (lastChar.compare(pathSep) != 0)
        folderPath += pathSep;

    int x = 0;
    for (uint32 i = 0; i < chunkCount; ++i, ++x)
    {
        sJob job;
        job.sourceDocument = document;
        job.deepCopy = deepCopy;
        job.outputType = outputType;
        job.chunkSize = chunkSize;
        for (uint32 j = 0; j < chunkSize; j++)
        {
            job.clonedPages.append(document->getPage(i * chunkSize + j)->clone());
        }

        std::wstring fullOutputPath(
            folderPath + 
            _outputFile.c_str());
        std::wstring fullPath(
            fullOutputPath +
            pageIndex(i * chunkSize + 1, chunkSize) +
            std::wstring(extensionFromFormat(outputType).c_str())
        );

        job.outputFile = fullPath.c_str();

        jobs[x].push_back(job);
        if (x == availableWorkers - 1)
        {
            x = -1;
        }
    }

    if (finalChunkSize)
    {
        sJob job;
        job.sourceDocument = document;
        job.deepCopy = deepCopy;
        job.outputType = outputType;
        job.chunkSize = finalChunkSize;
        for (uint32 j = 0; j < finalChunkSize; j++)
        {
            job.clonedPages.append(document->getPage(chunkCount * chunkSize + j)->clone());
        }

        const std::wstring fullOutputPath(
            folderPath +
            _outputFile.c_str());
        std::wstring fullPath(
            fullOutputPath +
            pageIndex(chunkCount * chunkSize + 1, finalChunkSize) +
            std::wstring(extensionFromFormat(outputType).c_str())
        );

        job.outputFile = fullPath.c_str();

        jobs[x].push_back(job);
        if (x == availableWorkers - 1)
        {
            x = -1;
        }
    }
    
    // Spawn worker threads
    for (unsigned int i = 0; i < threadCount; ++i)
    {
        workers[i] = thread(&threadRunner, mako, &jobs[i]);
    }

    // Run final (or only) job on the main thread (safe to do, as output is straightforward, autonomous process and does not need to be "listened" to)
    threadRunner(mako, &jobs[threadCount]);

    // Wait for the worker threads to finish
    for (unsigned int i = 0; i < threadCount; ++i)
    {
        if (workers[i].joinable())
        {
            workers[i].join();
        }
    }

    delete[] jobs;
    delete[] workers;
}

// Program entry point
#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);
#else
int main(int argc, char *argv[])
{
#endif

    std::string p(1, PATH_SEP_CHAR);
    String pathSeparator = U8StringToString(p.c_str());

    try
    {
        // Check number of arguments
        if (argc < 2)
        {
            usage();
            return 1;
        }

        // Copy command line parameters to a Mako string array
        CEDLStringVect argString;
        for (int i = 1; i < argc; i++)
        {
#ifdef _WIN32
            argString.append(argv[i]);
#else
            argString.append(U8StringToString(U8String(argv[i])));
#endif
        }

        // Populate parameters from argument values
        sParameters params = parseParams(argString);

        // Create JawsMako instance
        const IJawsMakoPtr jawsMako = IJawsMako::create();
        IJawsMako::enableAllFeatures(jawsMako);

        // Check the input file exists
        if (!fs::exists(params.inputFullPath))
        {
            std::wcerr << L"File " << params.inputFullPath << L" does not exist. " << std::endl;
            return 1;
        }

        // Check output folder exists; create if not
        if (params.outputPath.size())
        {
            if (!fs::exists(params.outputPath))
            {
                int nError = 0;
#if defined(_WIN32)
                wchar_t const* sPath = params.outputPath.c_str();
                nError = _wmkdir(sPath);
#else
                mode_t nMode = 0733;
                U8String u8sPath = StringToU8String(params.outputPath);
                char const* sPath = u8sPath.c_str();
                nError = mkdir(sPath, nMode);
#endif
                if (nError != 0) {
                    std::wcerr << L"Unable to create folder " << params.outputPath << L". " << std::endl;
                    return 1;
                }
            }
        }

        // Timer
        const clock_t begin = clock();

        // Create an input
        IInputPtr input = IInput::create(jawsMako, params.inputType);
        if (params.inputType == eFFPDF && params.userPassword.size())
        {
            IPDFInputPtr pdfInput = obj2IPDFInput(input);
            if (pdfInput)
                pdfInput->setPassword(params.userPassword);
        }

        // Get the assembly from the input
        IDocumentAssemblyPtr assembly = input->open(params.inputFullPath);

        // Grab the document and page count
        IDocumentPtr document = assembly->getDocument();
        const uint32 pageCount = document->getNumPages();
        if (!params.chunkSize) params.chunkSize = 1;    // One PDF per page
        if (params.chunkSize > pageCount)
            params.chunkSize = pageCount;        // Copy all pages to a single output PDF

        // Output the document "chunks"
        dumpChunks(jawsMako, document, pageCount, params.chunkSize, params.outputPath, params.outputBasename, params.outputType, params.singleThread, params.deepCopy);

        const clock_t end = clock();
        const double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        std::wcout << L"Elapsed time: " << elapsed_secs << L" seconds." << std::endl;

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