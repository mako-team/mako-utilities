# Mako Splitter

Mako Splitter divides the source file into a number of separate files of equal size. It employs a threading pattern, in effect writing the output files in parallel.

```plain
Mako Splitter v1.2.X

   Makosplitter input.xxx [output.yyy] [parameter=setting] [parameter=setting] ...
 Where:
   input.xxx          source file from which to extract pages, where xxx is pdf, xps, pxl (PCL/XL) or pcl (PCL5).
   output.yyy         target file to write the output to, where yyy is pdf, xps, pxl or pcl.
                        If no output file is declared, <input>.pdf is assumed.
   parameter=setting  one or more settings, described below.

Parameters:
   pw=<password>      PDF password, if required to open the file.
   c=<chunk size>     The number of pages per output file (omitted or 0 means one file per page)
   f=yes|no           Create a folder to contain the output, named according to the output file name. Default is no folder.
   s=yes|no           Use a single thread (yes), otherwise multiple threads are used to write the output files, the default.
   d=yes|no           Use a deep copy of pages, ie copy bookmarks and form field metadata. May negatively impact performance.
                        Default is no.
```

## How it works

The program creates a list of 'jobs', each of which consists of a number of pages, determined by the 'chunk' size. These are scheduled by a threadrunner, initialised with the number of available threads. Jobs are scheduled until they are exhausted.

The final file written may have fewer pages than the chunk size if the number of pages in the source file cannot be evenly divided.

When a job is processed, a simple loop copies the pages over from the source to the target document, for example:

```C++
IDocumentAssemblyPtr assembly = IDocumentAssembly::create(mako);
IDocumentPtr document = IDocument::create(mako);
for (uint32 i = 0; i < chunkSize; i++)
{
   document->appendPage(clonedPages[i]);
}
assembly->appendDocument(document);
output->writeAssembly(assembly, outputFile);
```

### Deep copy with AppendPage()

The source document can be made available to the `appendPage()` call, eg the code above would look like:

```C++
document->appendPage(clonedPages[i], sourceDocument);
```

Doing so allows Mako to copy over related page information, for example bookmarks that target the page and form field metadata. Doing so may negatively impact performance. In MakoSplitter this behavior is controlled by a command-line parameter.

## Useful sample code

* Threading pattern
* Efficiently copying content from one document to another