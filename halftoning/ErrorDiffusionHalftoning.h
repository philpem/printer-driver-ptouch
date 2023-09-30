// -*- C++ -*-
// $Id: ErrorDiffusionHalftoning.h 4759 2008-06-19 19:02:27Z vbuzuev $

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


#ifndef h70F89562_C051_4c11_80E4_E5BE22A3C400
#define h70F89562_C051_4c11_80E4_E5BE22A3C400

#include "Halftoning.h"

//namespace dymo
namespace DymoPrinterDriver
{

class CErrorDiffusionHalftoning: public CHalftoneFilter
{
public:
  CErrorDiffusionHalftoning(image_t InputImageType, image_t OutputImageType, bool UsePrinterColorSpace = true);    
  virtual ~CErrorDiffusionHalftoning();

  virtual bool IsProcessLineSupported();

  virtual void ProcessLine(const buffer_t& InputLine, buffer_t& OutputLine);
  virtual void ProcessImage(const void* ImageData, size_t ImageWidth, size_t ImageHeight, size_t LineDelta, std::vector<buffer_t>& OutputImage);
  virtual void ProcessImage(const std::vector<buffer_t>& InputImage, std::vector<buffer_t>& OutputImage);

protected:
  size_t GetImageWidth();

private:

  size_t              ImageWidth_;    // image width in pixels
  std::vector<int>    Errors_;        // errors buffer
  std::vector<int>    GrayLine_;      // current line in gray scale color
  bool                UsePrinterColorSpace_; // if true then use 1 as black, 0 - as white; otherwise 0 is black 1 - white
};
    
}; // namespace

#endif

/*
 * End of "$Id: ErrorDiffusionHalftoning.h 4759 2008-06-19 19:02:27Z vbuzuev $".
 */
