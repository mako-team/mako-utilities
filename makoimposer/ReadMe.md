# Mako Imposer

Based on the _simpleexamples_ sample code that ships with the SDK, Mako Imposer creates a booklet imposition from the source file. The main difference is that the user can choose the page size of the output file, from a list that will be familiar to anybody who has used a PostScript printer driver on Windows, as the names and dimensions are taken from there. It also supports overprint simulation.

```plain
Mako Imposer v1.2.X

Usage:
   makoimposer input.pdf|xps|pxl|pcl [output.pdf|xps|pxl|pcl] [parameter=setting] [parameter=setting] ...
   parameter=setting  one or more settings, described below.

Parameters:
   input.xxx      source file from which to extract pages, where xxx is pdf, xps, pxl (PCL/XL) or pcl (PCL5)
   output.yyy     target file to write the output to, where yyy is pdf, xps, pxl or pcl.
                    If no output file is declared, <input>_booklet.pdf is assumed.
   pw=<password>  PDF password, if required to open the file.
   f=yes|no       Flatten transparency. Default is no, ie do not flatten transparency.
   o=yes|no       Simulate overprint. Default is no, ie do not simulate overprint.
   p=pagesize     Page size chosen from the list below. Default is A3.

10X11                   10X14                   11X17                   12X11
15X11                   9X11                    A2                      A3
A3_EXTRA                A4                      A4SMALL                 A4_EXTRA
A4_PLUS                 A5                      A5_EXTRA                A6
A_PLUS                  B4                      B5                      B5_EXTRA
B6_JIS                  B_PLUS                  CSHEET                  DBL_JAPANESE_POSTCARD
DSHEET                  ENV_10                  ENV_11                  ENV_12
ENV_14                  ENV_9                   ENV_B4                  ENV_B5
ENV_B6                  ENV_C3                  ENV_C4                  ENV_C5
ENV_C6                  ENV_C65                 ENV_DL                  ENV_INVITE
ENV_ITALY               ENV_MONARCH             ENV_PERSONAL            ESHEET
EXECUTIVE               FANFOLD_LGL_GERMAN      FANFOLD_STD_GERMAN      FANFOLD_US
FOLIO                   ISO_B4                  JAPANESE_POSTCARD       JENV_CHOU3
JENV_CHOU4              JENV_KAKU2              JENV_KAKU3              JENV_YOU4
LEDGER                  LEGAL                   LEGAL_EXTRA             LETTER
LETTER_EXTRA            LETTER_PLUS             NOTE                    P16K
P32K                    P32KBIG                 PENV_1                  PENV_10
PENV_2                  PENV_3                  PENV_4                  PENV_5
PENV_6                  PENV_7                  PENV_8                  PENV_9
QUARTO                  STATEMENT               TABLOID                 TABLOID_EXTRA
```

## How it works

For each output spread, the code works out the position, scaling and rotation factors required to place the source page into the new spread. These are progressively added to an `FMatrix`, for example:

```C++
FMatrix transform;
...
// Rotate counter-clockwise if landscape
transform = FMatrix(0.0, -1.0, 1.0, 0.0, 0.0, sourceWidth); 
...
// Creates a scaling matrix
FMatrix scaleMatrix(scale, 0.0, 0.0, scale, (targetWidth - scaledWidth) / 2.0, (targetHeight - scaledHeight) / 2.0);
transform.postMul(scaleMatrix); // Combines the two matrices
...
// Shifts the content if the page is on the right
transform.setDX(transform.dx() + targetWidth);
```

The code then creates a group (`IDOMGroup`) with that transform, and copies the content from the original page with a simple loop:

```C++
// Copy in all the source DOM into that group
IDOMNodePtr child = content->getFirstChild();
while (child)
{
    child->cloneTreeAndAppend(jawsMako, transformGroup);
    child = child->getNextSibling();
}
```

And finally adds the content to the spread

```C++
// Append the group to the spread
spread->appendChild(transformGroup);
```

## Useful sample code

* Use of a transform group to move the content into the correct position on the target page
* A C++ header that contains 76 standard page sizes expressed in Mako units (1/96<sup>th</sup> inch)
