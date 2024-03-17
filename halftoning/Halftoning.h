// -*- C++ -*-
// $Id: Halftoning.h 15960 2011-09-02 14:42:28Z pineichen $

// DYMO LabelWriter Drivers
// Copyright (C) 2008 Sanford L.P.

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#ifndef h4D098F6A_47C6_4e9d_BD74_2DC6034F8EEF
#define h4D098F6A_47C6_4e9d_BD74_2DC6034F8EEF

#include <stdlib.h>
#include "CommonTypedefs.h"

//namespace dymo
namespace DymoPrinterDriver
{

class CHalftoneFilter
{
public:
  // image format
  typedef enum
  {
    itBW,       // Black and White
    itXRGB,     // four bytes per pixel, 8 bits per color, msb is not used (default on MacOSX)
    itRGB,      // three bytes per pixel, 8 bits per color (default on CUPS)
  } image_t;

  typedef std::vector<buffer_t> image_buffer_t;

  CHalftoneFilter(image_t InputImageType, image_t OutputImageType);
  virtual ~CHalftoneFilter();


  // line-by-line interface
  virtual bool IsProcessLineSupported() = 0;
  virtual void ProcessLine(const buffer_t& InputLine, buffer_t& OutputLine) = 0;

  // full-image-at-once interface
  virtual void ProcessImage(const void* ImageData, size_t ImageWidth, size_t ImageHeight, size_t LineDelta, std::vector<buffer_t>& OutputImage) = 0;
  virtual void ProcessImage(const image_buffer_t& InputImage, image_buffer_t& OutputImage) = 0;

  image_t GetInputImageType();
  image_t GetOutputImageType();
    
  // convert RGB value to Gray Scale
  byte RGBToGrayScale(byte R, byte G, byte B);
    
  // pixelValue (0 - white, 1 - black)
  void SetPixelBW(buffer_t& buf, int pixelNo, int pixelValue);
    
  // based on inputImageType extract color component of current pixel
  void ExtractRGB(const buffer_t& InputLine, int PixelNo, byte& R, byte& G, byte& B);
  // same as previous but return colors as packed integer value
  int ExtractRGB(const buffer_t& InputLine, int PixelNo);
    
  // return imageWidth based on inputImageType and input line data
  size_t CalcImageWidth(const buffer_t& InputLine);
  // return buffer size needed to store an input line based on inputImageType
  size_t CalcBufferSize(size_t ImageWidth);
  // calc output buffer size
  size_t CalcOutputBufferSize(size_t ImageWidth);
    
private:
  image_t InputImageType_;
  image_t OutputImageType_;    
};


class EHalftoneError
{
public:
  typedef enum
  {
    heUnsupportedImageType = 1,
  } error_t;

  EHalftoneError(error_t ErrorCode);
        
  error_t GetErrorCode();

private:
  error_t ErrorCode_;
};

}

#endif

/*
 * End of "$Id: Halftoning.h 15960 2011-09-02 14:42:28Z pineichen $".
 */
