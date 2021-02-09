# Mako Watermarker

Adds a watermark, generated from a text string or a page from a PDF. Control is provided over angle and opacity.

```plain
Mako Watermarker v1.2.X

Usage:
   makowatermarker.exe <source.pdf> [<output.pdf>] [parameter=setting] [parameter=setting] ...

Where:
   input.pdf          source PDF
   output.pdf         PDF file to write the output to.
                        If no output file is specified, <source file>_watermarked.pdf is assumed.
   parameter=setting  one or more settings, described below.

Parameters:
   t=<watermark>      Text of watermark, eg 'Draft'. Surround with quotes if the text contains spaces
   f=<font name>      Font to use, eg 'Yu Gothic Bold'. Surround with quotes if the name contains spaces
   w=<watermark pdf>  Use first page of the specified PDF as the watermark content.
   a=<angle>          Angle from -180°(anti-clockwise) to +180°(clockwise) of rotation
                        If no angle is specified, a default of zero (ie horizontal) is assumed)
 The next four parameters control the color and opacity of the watermark text
   r=<red>            Red component % value in range 0 - 100. Default is zero
   g=<green>          Green component % value in range 0 - 100. Default is 80%
   b=<blue>           Blue component % value in range 0 - 100. Default is 80%
   o=<opacity>        Opacity % value in range 0 - 100. Default is 40%

   i=<yes|no>         Incremental save:
                        Y = use it (default)
                        N = do not use it; processing will take longer but may produce smaller output
```

## How it works

If the option  to create a text-based watermark is chosen, this code is used to create a group than can be added to the page.

```C++
// Choose a font. Choose Arial again as it's likely to be present.
// We'll get adventurous and choose bold though.
uint32 fontIndex; // In case the font is inside a TrueType collection
const IDOMFontPtr font = jawsMako->findFont(params.fontName, fontIndex);

// A brush for the watermark
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
   font, fontIndex, IDOMGlyphs::eStyleSimulationsNone, rotate);
const FRect glyphBounds = glyphs->getBounds();
IDOMGroupPtr group = IDOMGroup::create(jawsMako, FMatrix(), IDOMPathGeometry::create(jawsMako, glyphBounds));
group->appendChild(glyphs);
```
Alternatively, the content can be pulled from another file:

```C++
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
```

### Adding the watermark to the page

This is accomplished by calculating a suitable transform to scale the content to suit the size of a target page. The content is added to a form XObject (`IDOMForm`) as this is an efficient way to add the same content to multiple pages. 

Finally, the watermark is added to ever page, respecting the opacity setting:

```C++
// Apply the watermark to every page
for (uint32 pageNum = 0; pageNum < pageCount; pageNum++)
{
   // A FormInstance is needed to hold the form (one per page)
   IDOMFormInstancePtr formInstance = createInstance<IDOMFormInstance>(
      jawsMako, CClassID(IDOMFormInstanceClassID));
   formInstance->setOpacity(float(params.opacityValue / 100.0f));
   formInstance->setForm(watermark);
   page = document->getPage(pageNum);
   IDOMFixedPagePtr fixedPage = page->edit();
   fixedPage->appendChild(formInstance);
}
```

## Useful sample code

* Creating text content
* Reading content from a file
* Transforming content
* Using form XObjects to add the same content to several pages