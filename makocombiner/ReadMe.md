# Mako Combiner

Combines multiple Mako-supported PDL files into a single file.

```plain
Mako Combiner v1.2.X

Usage:
   makocombiner <source file 1.xxx> <source file 2.xxx> .. <source file n.xxx>
                Combines (merges) multiple files into a single file.
Parameters:
                <source file.xxx> where .xxx can be any of .pdf, .xps, .pxl (PCL/XL) or .pcl (PCL5)
                /n-m[;n-m]... indicates one or more page ranges to copy, eg 10-20;80;90-
                  - A range of n- means from n to end.
                  - Invalid page ranges are adjusted automatically or ignored.
                <filename>/o indicates the file is the output file.
                If no output file is declared, a default of 'Combined.xxx' will be used (where xxx matches the first named file).
 -or-
   makocombiner <source file list (text file)> [<output file>] (to combine a list of files into the output file)
```

## How it works

Primarily aimed at merging PDF files, Mako Combiner copies pages from each source file to a new `IDocument`, appended to a new `IDocumentAssembly`, which is written out as the combined document with `writeAssembly()`.

### Bookmarks

Additionally, Mako Combiner transfers the outline (the bookmarks) from the source the target documents. A top-level bookmark is added to the target document for each of the source documents that have bookmarks, and book marks that reference the copied pages are added one level down. Link targets are adjusted accordingly. The source file `BookMarkTreeNode.cpp` has some useful utility methods for dealing with bookmarks.

Note: The bookmark code is not absolutely necessary, as it's possible to add a reference to the source document in the call to appendPage(), for example:

```C++
IPagePtr sourcePage = sourceDocument->getPage(pageIndex);
document->appendPage(sourcePage, sourceDocument);
```

Doing so ensures bookmarks that refer to the copied page are added to the outline in the target document. This option also ensures that form field metadata is copied, which could make the difference between a form element, such as a checkbox, behaving correctly or not. However, it also slows down the process, so in this example the bookmarks are handled separately as described above.

### Named destinations

In Mako 4.6, support for PDF Named Destinations was added that this utility makes use of. Mako Combiner will copy named destinations from the source document that refer to the copied pages to the target document. This ensures hypertext links that refer to named destinations will still function correctly in the output document.

## Useful sample code

* Bookmarks
* Named destinations
