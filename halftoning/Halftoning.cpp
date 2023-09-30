// -*- C++ -*-
// $Id: Halftoning.cpp 4759 2008-06-19 19:02:27Z vbuzuev $

// DYMO LabelWriter Drivers
// Copyright (C) 2008 Sanford L.P.
// C linkages added 2023 by VintagePC <https://github.com/vintagepc>

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

#include "Halftoning.h"
#include "HalftoneHelper.h"
#include "ErrorDiffusionHalftoning.h"
#include "NonLinearLaplacianHalftoning.h"
#include <algorithm>
#include <assert.h>
#include <memory.h>
#include <vector>

//using namespace std;

//namespace dymo
namespace DymoPrinterDriver
{

CHalftoneFilter::CHalftoneFilter(image_t InputImageType, image_t OutputImageType):
  InputImageType_(InputImageType), OutputImageType_(OutputImageType)
{
}

CHalftoneFilter::~CHalftoneFilter()
{
}

CHalftoneFilter::image_t 
CHalftoneFilter::GetInputImageType()
{
  return InputImageType_;
}

CHalftoneFilter::image_t 
CHalftoneFilter::GetOutputImageType()
{
  return OutputImageType_;
}

byte 
CHalftoneFilter::RGBToGrayScale(byte R, byte G, byte B)
{
  // white should remain white
  if ((R == 255) && (G == 255) && (B == 255))
    return 255;
  else if ((R == 0) && (G == 0) && (B == 0))
    return 0;
  else
  {  
    int r = 0 + ((int(R) * 299) / 1000) + ((int(G) * 587) / 1000) + ((int(B) * 114) / 1000);
    if (r > 255)
      return 255;
    return byte(r);
  }    
}

// set pixel pixelNo to
// pixelValue (0 - white, 1 - black)
void
CHalftoneFilter::SetPixelBW(buffer_t& buf, int pixelNo, int pixelValue)
{
  if (pixelValue)
    buf[pixelNo / 8] |= (1 << (7 - pixelNo % 8));
  else
    buf[pixelNo / 8] &= ~(1 << (7 - pixelNo % 8));
}

void 
CHalftoneFilter::ExtractRGB(const buffer_t& InputLine, int PixelNo, byte& R, byte& G, byte& B)
{
  switch (InputImageType_)
  {
    case itXRGB:
      R = InputLine[4*PixelNo + 1];
      G = InputLine[4*PixelNo + 2];
      B = InputLine[4*PixelNo + 3];
      break;
    case itRGB:
      R = InputLine[3*PixelNo + 0];
      G = InputLine[3*PixelNo + 1];
      B = InputLine[3*PixelNo + 2];
      break;
    default:
      assert(0);
  }                        
}

size_t 
CHalftoneFilter::CalcImageWidth(const buffer_t& InputLine)
{
  switch (InputImageType_)
  {
    case itXRGB:
      return InputLine.size() / 4;
    case itRGB:
      return InputLine.size() / 3;
    default:
      assert(0);    
  }            

  return 0; // for MSVC compiler
}


size_t 
CHalftoneFilter::CalcBufferSize(size_t ImageWidth)
{
  switch (InputImageType_)
  {
    case itXRGB:
      return ImageWidth * 4;
    case itRGB:
      return ImageWidth * 3;
    default:
      assert(0);    
  }            

  return 0; // for MSVC compiler
}

size_t
CHalftoneFilter::CalcOutputBufferSize(size_t ImageWidth)
{
  switch (OutputImageType_)
  {
    case itBW:
      if (ImageWidth % 8 == 0)
        return ImageWidth / 8;
      else
        return ImageWidth / 8 + 1;
    default:
      assert(0);    
  }            

  return 0; // for MSVC compiler
}

int 
CHalftoneFilter::ExtractRGB(const buffer_t& InputLine, int PixelNo)
{
  switch (InputImageType_)
  {
    case itXRGB:
      return 
        (int(InputLine[4*PixelNo + 1]) << 16)
        || (int(InputLine[4*PixelNo + 2]) << 8)
        || (InputLine[4*PixelNo + 3] );
    case itRGB:        
      return 
        (int(InputLine[3*PixelNo + 0]) << 16)
        || (int(InputLine[3*PixelNo + 1]) << 8)
        || (InputLine[3*PixelNo + 2] );
    default:
      assert(0);        
  }    

  return 0; // for MSVC compiler
}

/////////////////////////////////////////////////////////////////////////
// EHalftoneError
/////////////////////////////////////////////////////////////////////////

EHalftoneError::EHalftoneError(error_t ErrorCode): ErrorCode_(ErrorCode)
{
}
        
EHalftoneError::error_t 
EHalftoneError::GetErrorCode()
{
  return ErrorCode_;
}


} // namespace

/*
 * End of "$Id: Halftoning.cpp 4759 2008-06-19 19:02:27Z vbuzuev $".
 */

 // C linkages for C++ classes:

using ErrDiff = DymoPrinterDriver::CErrorDiffusionHalftoning;
using NLL = DymoPrinterDriver::CNLLHalftoning;
using Filter = DymoPrinterDriver::CHalftoneFilter;
using DymoPrinterDriver::buffer_t;
using std::vector;
extern "C"
{
  extern int do_halftone_err_diff(unsigned char* buffer, int bufLen) 
  {
    ErrDiff H(Filter::itRGB, Filter::itBW);
    buffer_t input(buffer, buffer + bufLen), output;
    H.ProcessLine(input, output);
    memcpy(buffer, output.data() , output.size());
    return output.size();
  }


}