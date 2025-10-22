/* rastertoptch is a filter to convert CUPS raster data into a Brother
 * P-touch label printer command byte stream.
 *
 * Copyright (c) 2006  Arne John Glenstrup <panic@itu.dk>
 *
 * This file is part of ptouch-driver
 *
 * ptouch-driver is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ptouch-driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ptouch-driver; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * Please note: This software is not in any way related to Brother
 * Industries, Ltd., except that it is intended for use with their
 * products.  Any comments to this software should NOT be directed to
 * Brother Industries, Ltd.
 */

/** @file
 * This filter processes CUPS raster data, converting it into a byte
 * stream on stdout suitable for sending directly to a label printer
 * of the Brother P-touch family.
 *
 * @version 1.2
 * @author  Arne John Glenstrup <panic@itu.dk>
 * @date    2006

 * <h2>Invocation</h2>
 * The filter is invoked thus:
 *
 *         rastertoptch [options] {job-options}
 *
 * @param options      See "rastertoptch --help".
 * @param job-options  String representations of the job template
 *                     parameters, separated by spaces. Boolean attributes
 *                     are provided as "name" for true values and "noname"
 *                     for false values. All other attributes are provided
 *                     as "name=value".
 *
 * Available job-options (default values in [brackets]):
 *
 * @param PixelXfer=ULP|RLE|BIP  Use uncompressed line printing (ULP),
 *                               run-length encoding (RLE) or bit
 *                               image printing (BIP) when emitting
 *                               pixel data [ULP]
 * @param PrintQuality=High|Fast Use high quality or fast printing [High]
 * @param HalfCut                Perform half-cut (crack & peel) when
 *                               cutting [noHalfCut]
 * @param BytesPerLine=N         Emit N bytes per line [90]
 * @param Align=Right|Center     Pixel data alignment on tape [Right]
 * @param MediaType=Tape|Labels  Media Type
 * @param PrintDensity=1|...|5   Print density level: 1=light, 5=dark
 * @param ConcatPages            Output all pages in one page [noConcatPages]
 * @param SoftwareMirror         Make the filter mirror pixel data
 *                               if MirrorPrint is requested [noSoftwareMirror]
 * @param LabelPreamble          Emit preamble containing print quality,
 *                               roll/label type, tape width, label height,
 *                               and pixel lines [noLabelPreamble]
 *
 * Information about resolution, mirror print, negative
 * print, cut media, advance distance (feed) is extracted from the
 * CUPS raster page headers given in the input stream.
 *
 * LabelPreamble should usually not be used for the PT series printers.
 *
 * <h2>Output</h2>
 * Each invocation of this filter is one job, containing a number of
 * pages, each page containing a number of lines, each line consisting
 * of a number of pixel bytes.
 *
 * Output consists of job-related printer initialisation commands,
 * followed by a number of pages, each page consisting of page-related
 * commands, followed by raster line data.  Each page is followed by a
 * finish page or (after the final page) finish job command.
 *
 * The following printer command language, printer, and tape
 * information has been deduced from many sources, but is not official
 * Brother documentation and may thus contain errors.  Please send any
 * corrections based on actual experience with these printers to the
 * maintainer.
 *
 * <h3>Job-related commands</h3>
 * <table>
 * <tr><th>Byte sequence</th><th>Function</th><th>Description</th></tr>
 * <tr><td>ESC @ (1b 40)</td>
       <td>Initialise</td><td>Clear print buffer</td></tr>
 * <tr><td>ESC i D # (1b 69 44 ##)
 *     <td>Set print density</td>
 *     <td>bit 0-3: 0=no change, 1-5=density level</td></tr>
 * <tr><td>ESC i K # (1b 69 4b ##)
 *     <td>Set half cut</td>
 *     <td>bit 2: 0=full cut, 1=half cut</td></tr>
 * <tr><td>ESC i a ## (1b 69 52 ##)</td>
 *     <td>Set transfer mode (legacy: ESC i R ##)</td>
 *     <td>##: Transfer mode: 1=Raster mode</td></tr>
 * <tr><td>M ## (4d ##)</td>
 *     <td>Set compression</td>
 *     <td>##: Compression type: 2=RLE</td></tr>
 * </table>
 *
 * <h3>Page-related commands</h3>
 * <table>
 * <tr><th>Byte sequence</th><th>Function</th><th>Description</th></tr>
 * <tr><td>ESC i c #1 #2 #3 NUL #4 <br>(1b 63 #1 #2 #3 00 #4)
 *     <td>Set width & resolution</td>
 *     <td>360x360DPI: #1 #2 #4 = 0x84 0x00 0x00<br>
 *         360x720DPI: #1 #2 #4 = 0x86 0x09 0x01<br>
 *         #3: Tape width in mm</td></tr>
 * <tr><td>ESC i M # <br>(1b 69 4d ##)</td>
 *     <td>Set mode</td>
 *     <td>bit 0-4: Feed amount (default=large): 0-7=none, 8-11=small,
 *                  12-25=medium, 26-31=large<br>
 *         bit 6: Auto cut/cut mark (default=on): 0=off, 1=on<br>
 *         bit 7: Mirror print (default=off): 0=off, 1=on.
 *                (note that it seems that QL devices do not reverse the
 *                data stream themselves, but rely on the driver doing
 *                it!)</td></tr>
 * <tr><td>ESC i z #1 #2 #3 #4 #5 #6 NUL NUL NUL NUL<br>
 *         (1b 69 7a #1 #2 #3 #4 #5 #6 00 00 00 00)</td>
 *     <td>Set media & quality</td>
 *     <td>#1, bit 6: Print quality: 0=fast, 1=high<br>
 *         #2, bit 0: Media type: 0=continuous roll,
 *                                1=pre-cut labels<br>
 *         #3: Tape width in mm<br>
 *         #4: Label height in mm (0 for continuous roll)<br>
 *         #5 #6: Page consists of N=#5+256*#6 pixel lines</td></tr>
 * <tr><td>ESC i d #1 #2 <br>(1b 69 64 #1 #2)</td>
 *     <td>Set margin</td>
 *     <td>Set size of top and bottom margin to N=#1+256*#2 pixels</td></tr>
 * <tr><td>FF (0c)</td>
 *     <td>Form feed</td>
 *     <td>Print buffer data without ejecting.</td></tr>
 * <tr><td>SUB (1a)</td>
 *     <td>Eject</td>
 *     <td>Print buffer data and ejects.</td></tr>
 * </table>
 *
 * <h3>Line-related commands</h3>
 * <table>
 * <tr><th>Byte sequence</th><th>Function</th><th>Description</th></tr>
 * <tr><td>G #1 #2 ...data... <br>(47 #1 #2 ...data...)</td>
 *     <td>Send raster line</td>
 *     <td>data consists of
 *         N=#1+256*#2 bytes of RLE compressed raster data.
 *         </td></tr>
 * <tr><td>Z (5a)</td>
 *     <td>Advance tape</td><td>Print 1 empty line</td></tr>
 * <tr><td>g #1 #2 ...data... <br>(67 #1 #2 ...data...)</td>
 *     <td>Send raster line</td>
 *     <td>data consists of
 *         N=#2 bytes of uncompressed raster data.</td></tr>
 * <tr><td>ESC * ' #1 #2 ...data... <br>(1b 2a 27 #1 #2 ...data...)</td>
 *     <td>Bit image printing (BIP)</td>
 *     <td>Print N=#1+256*#2 lines of 24 pixels; data consists of 3*N
 *         bytes</td></tr>
 * </table>
 *
 * <h3>Compressed-data-related commands (RLE)</h3>
 * <table>
 * <tr><th>Byte sequence</th><th>Function</th><th>Description</th></tr>
 * <tr><td>#1 ...data...</td>
 *     <td>#1 >= 0: Print uncompressed</td>
 *     <td>data consists of 1+#1 uncompressed bytes</td></tr>
 * <tr><td>#1 #2</td>
 *     <td>#1 < 0: Print compressed</td>
 *     <td>#2 should be printed 1-#1 times</td></tr>
 * </table>
 * #1 is represented as a 2-complement signed integer.
 *
 * <h2>Printer model characteristics</h2>
 * The following table lists for each model what kind of cutter it has
 * (manual, auto, half cut), what kind of pixel data transfer mode it
 * requires, its resolution, number of print head pixels, number of
 * bytes of pixel data that must be transmitted per line (regardless
 * of actual tape width!), and what kinds of tape it can take.
 *
 * For PC models, pixel data must be centered, so narrow tapes require
 * padding raster data with zero bits on each side.  For QL models,
 * labels are left-aligned, so pixel data must be right aligned, so
 * narrow tapes require padding raster data with zero bits at the end.
 *
 * For PC-PT, only the central 24 pixels (= 3,4mm!) can be used for
 * pixel-based graphics.  It might be possible to print several strips
 * of 24 pixels side-by side by issuing CR and line-positioning
 * commands.  That is currently not supported, let alone attempted,
 * with this driver.
 *
 * <table>
 * <tr><th>Model    <th>Cutter  <th>Xfer<th>DPI<th>Pixels<th>Bytes<th>Tape
 * <tr><td>QL-500   <td>manual   <td>ULP<td>300<td>720<td>90<td>DK12-62mm
 * <tr><td>QL-550   <td>auto     <td>ULP<td>300<td>720<td>90<td>DK12-62mm
 * <tr><td>QL-570   <td>auto     <td>ULP<td>300<td>720<td>90<td>DK12-62mm
 * <tr><td>QL-650TD <td>auto     <td>ULP<td>300<td>720<td>90<td>DK12-62mm
 * <tr><td>QL-700   <td>auto     <td>ULP<td>300<td>720<td>90<td>DK12-62mm
 * <tr><td>PT-PC    <td>auto     <td>BIP<td>180<td>128<td> 3<td>TZ6-24mm
 * <tr><td>PT-18R   <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-18mm
 * <tr><td>PT-550A  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-36mm
 * <tr><td>PT-P700  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ4-24mm
 * <tr><td>PT-P710BT<td>auto     <td>RLE<td>180<td>128<td>16<td>TZ4-24mm
 * <tr><td>PT-P900W <td>auto     <td>RLE<td>
 *                                      360x720<td>384<td>70<td>TZ4-36mm
 * <tr><td>PT-1500PC<td>manual   <td>RLE<td>180<td>112<td>14<td>TZ6-24mm
 * <tr><td>PT-1950  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-18mm
 * <tr><td>PT-1950VP<td>auto     <td>RLE<td>180<td>112<td>14<td>TZ6-18mm
 * <tr><td>PT-1960  <td>auto     <td>RLE<td>180<td> 96<td>12<td>TZ6-18mm
 * <tr><td>PT-2300  <td>auto     <td>RLE<td>180<td>112<td>14<td>TZ6-18mm
 * <tr><td>PT-2420PC<td>manual   <td>RLE<td>180<td>128<td>16<td>TZ6-24mm
 * <tr><td>PT-2430PC<td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-24mm
 * <tr><td>PT-2450DX<td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-24mm
 * <tr><td>PT-2500PC<td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-24mm
 * <tr><td>PT-2600  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ,AV6-24mm
 * <tr><td>PT-2610  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ,AV6-24mm
 * <tr><td>PT-2700  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ,TZ4-24mm
 * <tr><td>PT-3600  <td>auto/half<td>RLE<td>360<td>384<td>48<td>TZ,AV6-36mm
 * <tr><td>PT-9200DX<td>auto/half<td>RLE<td>360<td>384<td>48<td>TZ6-36mm
 * <tr><td>PT-9200PC<td>auto/half<td>RLE<td>360<td>384<td>48<td>TZ6-36mm
 * <tr><td>PT-9400  <td>auto/half<td>RLE<td>360<td>384<td>48<td>TZ6-36mm
 * <tr><td>PT-9500PC<td>auto/half<td>RLE<td>360<br>
                                        360x720<td>384<td>48<td>TZ,AV6-36mm
 * <tr><td>PT-9600  <td>auto/half<td>RLE<td>360<td>384<td>48<td>TZ,AV6-36mm
 * </table>
 *
 * <h2>Tape characteristics</h2>
 * <table>
 * <tr><th>Tape width
 *     <th colspan=2>Print area        <th>Margins</th><th>DPI</th></tr>
 * <tr><td>62mm<td>61.0mm<td>720pixels</td><td>0.5mm</td><td>300</td></tr>
 * <tr><td>36mm<td>27.1mm<td>384pixels</td><td>4.5mm</td><td>360</td></tr>
 * <tr><td>24mm<td>18.0mm<td>128pixels</td><td>3mm</td><td>180</td></tr>
 * <tr><td>18mm<td>12.0mm<td> 85pixels</td><td>3mm</td><td>180</td></tr>
 * <tr><td>12mm<td> 8.0mm<td> 57pixels</td><td>2mm</td><td>180</td></tr>
 * <tr><td> 9mm<td> 6.9mm<td> 49pixels</td><td>1mm</td><td>180</td></tr>
 * <tr><td> 6mm<td> 3.9mm<td> 28pixels</td><td>1mm</td><td>180</td></tr>
 * </table>
 *
 * <h2>Notes</h2>
 * - Pixels bytes sent are printed from right to left, with bit 7
 *   rightmost!
 * - Bit image printing (BIP) using "ESC * ' #1 #2 ...data..."
 *   probably only works for the PT-PC model.
 * - QL Printer documentation might state that the print area is less
 *   than 61mm, which is probably to ensure that printed pixels stay
 *   within the tape even if it is not precisely positioned.  The
 *   print head really IS 720 pixels.
 * - The Brother Raster Command Reference for various printers document that
 *   the "ESC i a 0x01" command sequence should be used to switch printers into
 *   raster mode.  That's the command sequence at least some of the Windows
 *   drivers use (tested on PT-P700 and PT-P900W).  It's unclear which (if any)
 *   printers require "ESC i R 0x01", so keep using that for old printers and
 *   switch to "ESC i a 0x01" for printers documented to use "ESC i a 0x01".
 */

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>
#include <float.h>
#include <cups/raster.h>
#include <cups/cups.h>
#include <limits.h>
#include <stdbool.h>
#include <getopt.h>
#include <libgen.h>

static const char* progname;

/** Length of a PostScript point in mm */
#define MM_PER_PT    (25.4 / 72.0)
/** Printer code: Eject */
#define PTC_EJECT    0x1a
/** Printer code: Form feed */
#define PTC_FORMFEED 0x0c

/** ASCII escape value */
#define ESC 0x1b

/**
 * Pixel transfer mode type.
 */
typedef enum {
  ULP, /**< Uncompressed line printing */
  RLE, /**< Run-length encoding        */
  BIP, /**< Bit image printing         */
} xfer_t;

/**
 * Pixel data alignment type.
 */
typedef enum {RIGHT, CENTER} align_t;
typedef enum {TAPE, LABELS} media_t;

/** CUPS Raster line buffer.                             */
unsigned char* buffer;
/** Buffer holding line data to emit to the printer.     */
unsigned char* emit_line_buffer;
/** Buffer holding RLE line data to emit to the printer. */
unsigned char* rle_buffer = NULL;
/** Pointer to first free pos in rle_buffer.             */
unsigned char* rle_buffer_next = NULL;
/** Size of rle_buffer.                                  */
unsigned long rle_alloced = 0;
/** Number of empty lines (input data only zeros) waiting to be stored */
int empty_lines = 0;
/** Number of pixel lines waiting to be emitted.         */
unsigned lines_waiting = 0;
/** Threshold for flushing waiting lines to printer.     */
unsigned max_lines_waiting = INT_MAX;

struct progress {
  unsigned int page;
  unsigned int height;
  unsigned int completed;
} progress;

void report_progress (int signal) {
  static struct progress old;
  char buffer[64];
  int len;

  if ((old.page == progress.page &&
       old.completed == progress.completed) ||
      progress.height == 0)
    return;
  old = progress;

  len = snprintf (buffer, sizeof (buffer),
		  "INFO: printing page %u, %u%% done\n",
		  progress.page,
		  progress.completed * 100 / progress.height);
  if (len > 0) {
    if (write (2, buffer, len) != len)
      /* ignore*/ ;
  }
}

/**
 * Struct type for holding all the job options.
 */
typedef struct {
  xfer_t pixel_xfer;    /**< pixel transfer mode                  */
  cups_bool_t print_quality_high; /**< print quality is high      */
  bool auto_cut;        /**< auto cut                             */
  bool half_cut;        /**< half cut                             */
  bool cut_mark;        /**< cut mark                             */
  int cut_label;        /**< cut each N label                     */
  bool chain_printing;  /**< chain printing                       */
  bool mirror_print;    /**< mirror print                         */
  bool pt_series;       /**< pt series printer                    */
  bool ql_series;       /**< ql series printer                    */
  int bytes_per_line;   /**< bytes per line (print head width)    */
  align_t align;        /**< pixel data alignment                 */
  media_t media;        /**< media type (TAPE or LABELS)          */
  bool software_mirror; /**< mirror pixel data if mirror printing */
  int print_density;    /**< printing density (1=light, ..., 5=dark, 0=don't change)    */
  int legacy_xfer_mode; /**< legacy transfer mode (-1 = don't set)       */
  int xfer_mode;        /**< transfer mode (-1 = don't set)       */
  bool label_preamble;  /**< emit ESC i z ...                     */
  bool label_recoyery;  /**< set PI_RECOVER flag                  */
  bool last_page_flag;  /**< flag last page in ESC i z            */
  bool legacy_hires;
  bool concat_pages;    /**< remove interlabel margins            */
  float min_margin;     /**< minimum top and bottom margin        */
  float margin;         /**< top and bottom margin                */
  int status_notification; /**< automatic status notification     */
  unsigned int page;    /**< The current page number              */
  bool last_page;       /**< This is the last page                */
} job_options_t;

/**
 * Parse command-line cups options
 * @param cups_options  cups option string
 * @param options       default options to use
 * @return              parsed options
 */
job_options_t
parse_job_options (const char* str) {
  job_options_t options = {
    /* pixel_xfer */ RLE,
    /* print_quality_high */ true,
    /* auto_cut */ false,
    /* half_cut */ false,
    /* cut_mark */ false,
    /* cut_label */ -1,
    /* chain_printing */ true,
    /* mirror_print */ false,
    /* pt_series */ false,
    /* ql_series */ false,
    /* bytes_per_line */ 90,
    /* align */ RIGHT,
    /* media */ TAPE,
    /* software_mirror*/ false,
    /* print_density */ 0,
    /* legacy_xfer_mode (don't set) */ -1,
    /* xfer_mode (don't set) */ -1,
    /* label_preamble */ false,
    /* label_recovery */ false,
    /* last_page_flag */ false,
    /* legacy_hires */ false,
    /* concat_pages */ false,
    /* min_margin */ 0.0,
    /* margin */ 0.0,
    /* status_notification (don't set) */ -1,
  };

  struct int_option {
    const char *name;
    int *value;
    int min;
    int max;
  };
  struct int_option int_options [] = {
    { "BytesPerLine", &options.bytes_per_line, 1, 255 },
    { "CutLabel", &options.cut_label, 0, 255 },
    { "PrintDensity", &options.print_density, 0, 5 },
    { "LegacyTransferMode", &options.legacy_xfer_mode, 0, 255 },
    { "TransferMode", &options.xfer_mode, 0, 255 },
    { "StatusNotification", &options.status_notification, 0, 1 },
    { }
  };

  struct bool_option {
    const char *name;
    bool *value;
  };
  struct bool_option bool_options [] = {
    { "AutoCut", &options.auto_cut },
    { "ChainPrinting", &options.chain_printing },
    { "ConcatPages", &options.concat_pages },
    { "CutMark", &options.cut_mark },
    { "HalfCut", &options.half_cut },
    { "LabelPreamble", &options.label_preamble },
    { "LabelRecovery", &options.label_recoyery },
    { "LastPageFlag", &options.last_page_flag },
    { "LegacyHires", &options.legacy_hires },
    { "MirrorPrint", &options.mirror_print },
    { "PT", &options.pt_series },
    { "QL", &options.ql_series },
    { "SoftwareMirror", &options.software_mirror },
    { }
  };

  struct float_option {
    const char *name;
    float *value;
    float min;
    float max;
  };
  struct float_option float_options [] = {
    { "MinMargin", &options.min_margin, 0, FLT_MAX },
    { "Margin", &options.margin, 0, FLT_MAX },
    { }
  };

  cups_option_t* cups_options;
  int num_options = cupsParseOptions (str, 0, &cups_options);
  while (num_options-- > 0) {
    const char *name = cups_options[num_options].name;
    const char *value = cups_options[num_options].value;

    if (strcasecmp (name, "PixelXfer") == 0) {
      if (value) {
	if (strcasecmp (value, "RLE") == 0) {
	  options.pixel_xfer = RLE;
	  continue;
	}
	if (strcasecmp (value, "BIP") == 0) {
	  options.pixel_xfer = BIP;
	  continue;
	}
	if (strcasecmp (value, "ULP") == 0) {
	  options.pixel_xfer = ULP;
	  continue;
	}
      }
      fprintf (stderr, "ERROR: The value of %s "
	       "must be RLE, BIP or ULP\n", name);
      exit (2);
      continue;
    }
    if (strcasecmp (name, "PrintQuality") == 0) {
      if (value) {
	if (strcasecmp (value, "High") == 0) {
	  options.print_quality_high = true;
	  continue;
	}
	if (strcasecmp (value, "Fast") == 0) {
	  options.print_quality_high = false;
	  continue;
	}
      }
      fprintf (stderr, "ERROR: The value of %s "
	       "must be High or Fast\n", name);
      exit (2);
      continue;
    }
    if (strcasecmp (name, "Align") == 0) {
      if (value) {
	if (strcasecmp (value, "Right") == 0) {
	  options.align = RIGHT;
	  continue;
	}
	if (strcasecmp (value, "Center") == 0) {
	  options.align = CENTER;
	  continue;
	}
      }
      fprintf (stderr, "ERROR: the value of %s "
	       "must be Right or Center\n", name);
      exit (2);
      continue;
    }
    if (strcasecmp (name, "MediaType") == 0) {
      if (value) {
	if (strcasecmp (value, "Tape") == 0) {
	  options.media = TAPE;
	  continue;
	}
      }
      if (value) {
	if (strcasecmp (value, "Labels") == 0) {
	  options.media = LABELS;
	  continue;
	}
      }
      fprintf (stderr, "ERROR: the value of %s "
	       "must be Tape or Labels\n", name);
      exit (2);
      continue;
    }

    struct int_option *int_option;
    for (int_option = int_options; int_option->name; int_option++) {
      if (strcasecmp (name, int_option->name) == 0) {
	if (value) {
	  errno = 0;
	  char *rest;
	  long v = strtol (value, &rest, 0);
	  if (!errno && rest != value && *rest == '\0' &&
	      v >= int_option->min && v <= int_option->max) {
	    *int_option->value = v;
	    break;
	  }
	}
	fprintf (stderr, "%s\n", value);
	fprintf (stderr, "ERROR: The value of %s must be an "
			 "integer between %d and %d\n",
		 name, int_option->min, int_option->max);
	exit (2);
      }
    }
    if (int_option->name)
      continue;

    struct bool_option *bool_option;
    for (bool_option = bool_options; bool_option->name; bool_option++) {
      if (strcasecmp (name, bool_option->name) == 0) {
	*bool_option->value = strcasecmp (value, "true") == 0;
	break;
      }
    }
    if (bool_option->name)
      continue;

    struct float_option *float_option;
    for (float_option = float_options; float_option->name; float_option++) {
      if (strcasecmp (name, float_option->name) == 0) {
	if (value) {
	  errno = 0;
	  char *rest;
	  float v = strtof (value, &rest);
	  if (!errno && rest != value && *rest == '\0' &&
	      v >= float_option->min && v <= float_option->max) {
	    *float_option->value = v;
	    break;
	  }
	}
	fprintf (stderr, "%s\n", value);
	fprintf (stderr, "ERROR: The value of %s must be an "
			 "integer between %f and %f\n",
		 name, float_option->min, float_option->max);
	exit (2);
      }
    }
    if (float_option->name)
      continue;

    fprintf (stderr, "ERROR: Unknown option %s\n", name);
    exit (2);
  }

  /* Release memory allocated for CUPS options struct */
  cupsFreeOptions (num_options, cups_options);
  return options;
}

void cancel_job (int signal);
/**
 * Prepare for a new page by setting up signalling infrastructure and
 * memory allocation.
 * @param cups_buffer_size    Required size of CUPS raster line buffer
 * @param device_buffer_size  Required size of device pixel line buffer
 */
void
page_prepare (unsigned cups_buffer_size, unsigned device_buffer_size) {
  /* Set up signalling to handle print job cancelling */
#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
  struct sigaction action;
#endif

#ifdef HAVE_SIGSET
  sigset (SIGTERM, cancel_job);
#elif defined(HAVE_SIGACTION)
  memset (&action, 0, sizeof (action));
  sigemptyset (&action.sa_mask);
  action.sa_handler = cancel_job;
  sigaction (SIGTERM, &action, NULL);
#else
  signal (SIGTERM, cancel_job);
#endif

  /* Allocate line buffer */
  buffer = malloc (cups_buffer_size);
  emit_line_buffer = malloc (device_buffer_size);
  if (!buffer || !emit_line_buffer) {
    fprintf
      (stderr,
       "ERROR: Cannot allocate memory for raster line buffer\n");
    exit (1);
  }
}

/**
 * Clean up signalling and memory after emitting a page
*/
void
page_end () {
#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
  struct sigaction action;
#endif

#ifdef HAVE_SIGSET
  sigset (SIGTERM, SIG_IGN);
#elif defined(HAVE_SIGACTION)
  memset (&action, 0, sizeof (action));
  sigemptyset (&action.sa_mask);
  action.sa_handler = SIG_IGN;
  sigaction (SIGTERM, &action, NULL);
#else
  signal (SIGTERM, SIG_IGN);
#endif
  /* Release line buffer memory */
  free (buffer);
  free (emit_line_buffer);
}

/**
 * Cancel print job.
 */
void
cancel_job (int signal) {
  /* Emit page end & eject marker */
  putchar (PTC_EJECT);
  page_end ();
  if (rle_buffer) free (rle_buffer);
  exit (0);
}

/**
 * Emit printer command codes at start of print job.
 * This function does not emit P-touch page specific codes.
 * @param job_options   Job options
 */
void
emit_job_cmds (job_options_t* job_options) {
  /* Send 350 bytes of NULL to clear print buffer in case an error occurred
   * previously. The printer ignores 0x00 bytes if it's waiting for a command.
   */
  int i;
  for (i=0; i<350; i++) {
    putchar(0x00);
  }
  /* Initialise printer */
  putchar (ESC); putchar ('@');
  /* Emit transfer mode selection command if required */
  int legacy_xfer_mode = job_options->legacy_xfer_mode;
  if (legacy_xfer_mode >= 0 && legacy_xfer_mode < 0x100) {
    putchar (ESC); putchar ('i'); putchar ('R'); putchar (legacy_xfer_mode);
  }
  int xfer_mode = job_options->xfer_mode;
  if (xfer_mode >= 0 && xfer_mode < 0x100) {
    putchar (ESC); putchar ('i'); putchar ('a'); putchar (xfer_mode);
  }
  if (job_options->status_notification != -1) {
    putchar (ESC); putchar ('i'); putchar ('!'), putchar (job_options->status_notification);
  }
}

/**
 * Emit quality, roll fed media, and label size command codes.
 * @param job_options      Current job options
 * @param header           Current page header
 * @param image_height_px  Number of pixel lines in current page
 */
void
emit_quality_rollfed_size (job_options_t* job_options,
                           cups_page_header2_t* header,
                           unsigned image_height_px) {
  const unsigned char PI_KIND = 0x02;   // Paper type (roll fed media bit) is valid
  const unsigned char PI_WIDTH = 0x04;  // Paper width is valid
  const unsigned char PI_LENGTH = 0x08; // Paper length is valid
  const unsigned char PI_QUALITY = 0x40;
  const unsigned char PI_RECOVER = 0x80;

  unsigned char valid = PI_WIDTH;
  if (job_options->label_recoyery)
    valid |= PI_RECOVER;
  /* Get tape width in mm */
  unsigned int tape_width_mm = lrint (header->cupsPageSize [0] * MM_PER_PT);
  if (tape_width_mm > 0xff) {
    fprintf (stderr,
             "ERROR: Page width (%umm) exceeds 255mm\n",
             tape_width_mm);
    tape_width_mm = 0xff;
  }
  unsigned char media_type = 0;
  unsigned int tape_length_mm = 0;
  if (job_options->ql_series) {
    if (job_options->print_quality_high == CUPS_TRUE)
      valid |= PI_QUALITY;
    valid |= PI_KIND;
    switch (job_options->media) {
    case TAPE:
      media_type = 0x0A;
      break;
    case LABELS:
      media_type = 0x0B;
      valid |= PI_LENGTH;
      tape_length_mm = lrint (header->cupsPageSize [1] * MM_PER_PT);
      break;
    }
    if (tape_length_mm > 0xff) {
      fprintf (stderr,
	       "ERROR: Page height (%umm) exceeds 255mm; use continuous-length tape\n",
	       tape_length_mm);
      tape_length_mm = 0xff;
    }
  }
  if (job_options->pt_series) {
    /*
     * On PT series printers, the media type must be set to 0x09 for
     * high-resolution and draft printing, but not for normal resolution
     * printing.
     */
    if (header->HWResolution [0] == 360 &&
	(header->HWResolution [1] == 180 ||
	 header->HWResolution [1] == 720)) {
      valid |= PI_KIND;
      media_type = 0x09;
    }
  }
  unsigned char which_page = job_options->page > 1;
  if (job_options->last_page_flag && job_options->last_page)
    which_page = 2;
  /* Combine & emit printer command code */
  putchar (ESC); putchar ('i'); putchar ('z');
  putchar (valid);
  putchar (media_type);
  putchar (tape_width_mm & 0xff);
  putchar (tape_length_mm);
  putchar (image_height_px & 0xff);
  putchar ((image_height_px >> 8) & 0xff);
  putchar ((image_height_px >> 16) & 0xff);
  putchar ((image_height_px >> 24) & 0xff);
  putchar (which_page);
  putchar (0x00);   // n10, always 0
}

/**
 * Emit printer command codes at start of page for options that have
 * changed.
 * @param job_options       Job options
 * @param header            Current page header
 */
void
emit_page_cmds (job_options_t* job_options,
		cups_page_header2_t* header) {
  float pt2px = header->HWResolution [1] / 72.0;
  int tape_width_mm = -1;

  /* Emit print density selection command if required */
  int density = job_options->print_density;
  switch (density) {
  case 1: case 2: case 3: case 4: case 5:
    putchar (ESC); putchar ('i'); putchar ('D'); putchar (density);
    break;
  default: break;
  }

  if (job_options->legacy_hires) {
    /* Set width and resolution */
    /* We only know how to select 360x360DPI or 360x720DPI */
    if (header->HWResolution [0] == 360
	&& (header->HWResolution [1] == 360
	    || header->HWResolution [1] == 720)) {
      /* Get tape width in mm */
      tape_width_mm = lrint (header->cupsPageSize [0] * MM_PER_PT);
      if (tape_width_mm > 0xff) {
	fprintf (stderr,
		 "ERROR: Page width (%umm) exceeds 255mm\n",
		 tape_width_mm);
	tape_width_mm = 0xff;
      }
      /* Emit printer commands */
      putchar (ESC); putchar ('i'); putchar ('c');
      if (header->HWResolution [1] == 360) {
	putchar (0x84); putchar (0x00); putchar (tape_width_mm & 0xff);
	putchar (0x00); putchar (0x00);
      } else {
	putchar (0x86); putchar (0x09); putchar (tape_width_mm & 0xff);
	putchar (0x00); putchar (0x01);
      }
    }
  }

  char various_mode = 0;
  if (job_options->auto_cut || job_options->cut_mark)
    various_mode |= 0x40;
  if (job_options->mirror_print && !job_options->software_mirror)
    various_mode |= 0x80;
  putchar (ESC); putchar ('i'); putchar ('M'); putchar (various_mode);

  char advanced_mode = 0;
  if (!job_options->legacy_hires) {
    if (header->HWResolution [0] == 360) {
      if (header->HWResolution [1] == 180)
	advanced_mode |= 0x01;  /* draft printing */
      if (header->HWResolution [1] == 720)
	advanced_mode |= 0x40;  /* hires printing */
    }
    if (header->HWResolution [0] == 300) {
      if (header->HWResolution [1] == 600)
	advanced_mode |= 0x40;  /* hires printing */
    }
  }
  if (job_options->half_cut)
    advanced_mode |= 0x04;
  if (!job_options->chain_printing)
    advanced_mode |= 0x08;
  putchar (ESC); putchar ('i'); putchar ('K'); putchar (advanced_mode);

  if (job_options->cut_label != -1) {
    putchar (ESC); putchar ('i'); putchar('A'); putchar (job_options->cut_label);
  }

  float margin = 0.0;
  if (job_options->media != LABELS)
    margin += job_options->min_margin + job_options->margin;
  unsigned feed = lrint (margin * pt2px);
  putchar (ESC); putchar ('i'); putchar ('d');
  putchar (feed & 0xff); putchar ((feed >> 8) & 0xff);

  /* Set pixel data transfer compression */
  if (job_options->pixel_xfer == RLE) {
    putchar ('M'); putchar (0x02);
  }

  /* Emit number of raster lines to follow if using BIP */
  if (job_options->pixel_xfer == BIP) {
    unsigned image_height_px = lrint (header->cupsPageSize [1] * pt2px);
    putchar (ESC); putchar (0x2a); putchar (0x27);
    putchar (image_height_px & 0xff);
    putchar ((image_height_px >> 8) & 0xff);
  }
}

/** mirror [i] = bit mirror image of i.
 * I.e., (mirror [i] >> j) & 1 ==  (i >> (7 - j)) & 1 for 0 <= j <= 7
 */
const unsigned char mirror [0x100] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

/**
 * Generate a buffer of pixel data ready to emit.
 * Requirement: buflen + right_padding_bytes
 *              + (shift > 0 ? 1 : 0) <= bytes_per_line
 * @param in_buffer           Buffer containing raster data in
 *                            left-to-right order
 * @param out_buffer          Buffer for returning generated line in
 *                            right-to-left order; must be
 *                            bytes_per_line long
 * @param buflen              in_buffer length
 * @param bytes_per_line      Number of pixel bytes to generate
 * @param right_padding_bytes Number of zero bytes to pad
 *                            with to the right of pixels
 * @param shift               Number of bits to shift left
 *                            If do_mirror is false and shift < 0
 *                            Then shift right -shift bits
 * @param do_mirror           Mirror in_buffer pixel data
 * @param xormask             The XOR mask for negative printing
 * @return                    0 if entire line is empty (zeros)
 *                            nonzero if line contains nonzero pixels
 */
static inline int
generate_emit_line (unsigned char* in_buffer,
                    unsigned char* out_buffer,
                    int buflen,
                    unsigned char bytes_per_line,
                    int right_padding_bytes,
                    int shift,
                    int do_mirror,
                    unsigned char xormask) {
  /* Generate right padding zero bytes */
  memset (out_buffer, xormask, right_padding_bytes);
  unsigned int nonzero = 0;
  int j = right_padding_bytes;
  /* Copy pixel data from in_buffer to out_buffer, */
  /* shifted and mirrored if required              */
  unsigned int box = 0; /* Box for shifting pixel data left */
  int i;
  if (do_mirror)
    if (shift) {
      for (i = 0; i < buflen; i++) {
        unsigned int data = in_buffer [i]; nonzero |= data;
        box |= data << shift;
        out_buffer [j++] = (box & 0xff) ^ xormask;
        box >>= 8;
      }
      out_buffer [j++] = box & 0xff;
    } else
      for (i = 0; i < buflen; i++) {
        unsigned char data = in_buffer [i]; nonzero |= data;
        out_buffer [j++] = data ^ xormask;
      }
  else
    if (shift) {
      if (buflen > 0) {
        if (shift < 0) {
          box = in_buffer [buflen - 1] >> -shift; nonzero |= box;
          shift += 8;
        } else {
          box = in_buffer [buflen - 1] << shift; nonzero |= box;
          out_buffer [j++] = (mirror [box & 0xff]) ^ xormask;
          box >>= 8;
        }
        for (i = buflen - 2; i >= 0; i--) {
          unsigned data = in_buffer [i]; nonzero |= data;
          box |= data << shift;
          out_buffer [j++] = (mirror [box & 0xff]) ^ xormask;
          box >>= 8;
        }
        out_buffer [j++] = (mirror [box & 0xff]) ^ xormask;
      }
    } else
      for (i = buflen - 1; i >= 0; i--) {
        unsigned char data = in_buffer [i]; nonzero |= data;
        out_buffer [j++] = (mirror [data]) ^ xormask;
      }
  /* Generate left padding bytes */
  memset (out_buffer + j, xormask, bytes_per_line - j);
  return nonzero != 0;
}

/**
 * Emit lines waiting in RLE buffer.
 * Resets global variable rle_buffer_next to rle_buffer,
 * and lines_waiting to zero.
 * @param job_options   Job options
 * @param header        Page header
 */
static inline void
flush_rle_buffer (job_options_t* job_options,
		  cups_page_header2_t* header) {
  if (lines_waiting > 0) {
    if (job_options->label_preamble)
      emit_quality_rollfed_size (job_options, header, lines_waiting);
    xfer_t pixel_xfer = job_options->pixel_xfer;
    int bytes_per_line = job_options->bytes_per_line;
    switch (pixel_xfer) {
    case RLE: {
      fwrite (rle_buffer, sizeof (char), rle_buffer_next - rle_buffer, stdout);
      break;
    }
    case ULP:
    case BIP: {
      unsigned char* p = rle_buffer;
      unsigned emitted_lines = 0;
      while (rle_buffer_next - p > 0) {
        if (pixel_xfer == ULP) {
	  /* ULP is only used by QL printers */
          putchar ('g'); putchar (0x00); putchar (bytes_per_line);
        }
        int emitted = 0;
        int linelen;
        switch (*p++) {
	case 'G':
	  linelen = *p++;
	  linelen += ((int)(*p++)) << 8;
	  goto expand_g_or_G;
        case 'g':
          linelen = ((int)(*p++)) << 8;
          linelen += *p++;
	expand_g_or_G:
          while (linelen > 0) {
            signed char l = *p++; linelen--;
            if (l < 0) { /* emit repeated data */
              char data = *p++; linelen--;
              emitted -= l; emitted++;
              for (; l <= 0; l++) putchar (data);
            } else { /* emit the l + 1 following bytes of data */
              fwrite (p, sizeof (char), l + 1, stdout);
              p += l; p++;
              linelen -= l; linelen--;
              emitted += l; emitted++;
            }
          }
          if (emitted > bytes_per_line)
            fprintf (stderr,
                     "ERROR: Emitted %d > %d bytes for one pixel line!\n",
                     emitted, bytes_per_line);
          /* No break; fall through to next case: */
        case 'Z':
          for (; emitted < bytes_per_line; emitted++) putchar (0x00);
          break;
        default:
          fprintf (stderr, "ERROR: Unknown RLE flag at %p: '0x%02x'\n",
                   p - 1, (int)*(p - 1));
        }
        emitted_lines++;
      }
      break;
    }
    default:
      fprintf (stderr, "ERROR: Unknown pixel transfer mode: '%d'\n",
               pixel_xfer);
    }
    rle_buffer_next = rle_buffer;
    lines_waiting = 0;
  }
}

/**
 * Ensure sufficient memory available in rle buffer.
 * If rle buffer needs to be extended, global variables rle_buffer and
 * rle_buffer_next might be altered.
 * @param job_options   Job options
 * @param header        Page header
 * @param bytes         Number of bytes required.
 */
static inline void
ensure_rle_buf_space (job_options_t* job_options,
                      cups_page_header2_t* header,
                      unsigned bytes) {
  unsigned long nextpos = rle_buffer_next - rle_buffer;
  if (nextpos + bytes > rle_alloced) {
    /* Exponential size increase avoids too frequent reallocation */
    unsigned long new_alloced = rle_alloced * 2 + 0x4000;
    void* p = NULL;
    if (new_alloced <= 1000000)
      p = realloc (rle_buffer, new_alloced * sizeof (char));
    if (p) {
      rle_buffer = p;
      rle_buffer_next = rle_buffer + nextpos;
      rle_alloced = new_alloced;
    } else { /* Gain memory by flushing buffer to printer */
      flush_rle_buffer (job_options, header);
      if (rle_buffer_next - rle_buffer + bytes > rle_alloced) {
        fprintf (stderr,
                 "ERROR: Out of memory when attempting to increase RLE "
                 "buffer from %ld to %ld bytes\n",
                 rle_alloced * sizeof (char),
                 new_alloced * sizeof (char));
        exit (-1);
      }
    }
  }
}

/** @def APPEND_MIXED_BYTES
 * Macro for appending mixed-bytes run to rle_buffer */
/** @def APPEND_REPEATED_BYTE
 * Macro for appending repeated-byte run to rle_buffer */
/**
 * Store buffer data in rle buffer using run-length encoding.
 * @param job_options   Job options
 * @param header        Page header
 * @param buf           Buffer containing data to store
 * @param buf_len       Length of buffer
 *
 * Global variable rle_buffer_next is a pointer into buffer for holding RLE data.
 * Must have room for at least 3 + buf_len + buf_len/128 + 1
 * bytes (ensured by reallocation).
 * On return, rle_buffer_next points to first unused buffer byte.
 *
 * This implementation enjoys the property that
 * the resulting RLE is at most buf_len + buf_len/128 + 1 bytes
 * long, because:
 * # a repeated-byte run has a repeat factor of at least 3
 * # two mixed-bytes runs never follow directly after each other,
 *   unless the first one is 128 bytes long
 * The first property ensures that a repeated-run output sequence is
 * always at least 1 byte shorter than the input sequence it
 * represents.  This combined with the second property means that only
 * - a terminating mixed-bytes run, and
 * - a mixed-bytes run of 128 bytes
 * can cause the RLE representation to be longer (by 1 byte) than the
 * corresponding input sequence in buf.
 */
static inline void
RLE_store_line (job_options_t* job_options,
		cups_page_header2_t* header,
                const unsigned char* buf, unsigned buf_len) {
  ensure_rle_buf_space (job_options, header,
                        4 + buf_len + buf_len / 128);
  unsigned char* rle_next = rle_buffer_next + 3;
  /* Make room for 3 initial meta data bytes, */
  /* written when actual length is known      */
  const unsigned char* buf_end = buf + buf_len; /* Buffer end */
  const unsigned char* mix_start;  /* Start of mixed bytes run */

  const unsigned char* rep_start;  /* End + 1 of mixed bytes run,
                                      and start of repeated byte run */
  const unsigned char* next;       /* Next byte pointer,
                                      and end + 1 of repeated byte run */
  unsigned char next_val;          /* Next byte value to consider */
  unsigned char rep_val;           /* Repeated byte value */
  unsigned char nonzero = 0;       /* OR of all buffer bytes */

#define APPEND_MIXED_BYTES                        \
        if (mix_len > 128) mix_len = 128;         \
        *rle_next++ = mix_len - 1;                \
        memcpy (rle_next, mix_start, mix_len);    \
        rle_next += mix_len;
#define APPEND_REPEATED_BYTE                      \
        unsigned rep_len = next - rep_start;      \
        *rle_next++ = (signed char)(1 - rep_len); \
        *rle_next++ = rep_val;

  for (mix_start = rep_start = next = buf, rep_val = next_val = *next;
       next != buf_end;
       next++, next_val = *next) {
    /* Loop invariants at this point:
     * 1) [mix_start..rep_start - 1] contains mixed bytes waiting
     *    to be appended to rle_buffer,
     * 2) [rep_start..next - 1] contains repeats of rep_val
     *    waiting to be appended to rle_buffer
     * 3) If next - rep_start > 2 then mix_start == rep_start
     * 4) next - rep_start <= 129
     * 5) rep_start - mix_start < 128
     * 6) [rle_buffer_next..rle_next - 1] = RLE ([buf..mix_start - 1])
     * 7) rep_val = *rep_start
     * 8) next_val = *next
     */
    nonzero |= next_val;
    if (next - rep_start >= 129) {
      /* RLE cannot represent repeated runs longer than 129 bytes */
      APPEND_REPEATED_BYTE;
      rep_start += rep_len;
      rep_val = *rep_start;
      mix_start = rep_start;
    }
    if (next_val == rep_val) { /* Run of repeated byte values */
      if (next - rep_start == 2) {
        unsigned mix_len = rep_start - mix_start;
        if (mix_len > 0) {
          APPEND_MIXED_BYTES;
          mix_start = rep_start;
        }
      }
    } else {
      if (next - rep_start > 2) { /* End of repeated run found */
        APPEND_REPEATED_BYTE;
        mix_start = next;
      }
      rep_start = next;
      rep_val = next_val;
      unsigned mix_len = rep_start - mix_start;
      if (mix_len >= 128) {
        /* RLE cannot represent mixed runs longer than 128 bytes */
        APPEND_MIXED_BYTES;
        mix_start += mix_len;
      }
    }
  }
  /* Handle final bytes */
  if (next - rep_start > 2) { /* Handle final repeated byte run */
    APPEND_REPEATED_BYTE;
    mix_start = next;
  }
  rep_start = next;
  unsigned mix_len = rep_start - mix_start;
  if (mix_len > 0) { /* Handle any remaining final mixed run */
    APPEND_MIXED_BYTES;
    mix_start += mix_len;
  }
  mix_len = rep_start - mix_start;
  if (mix_len > 0) { /* Case where final mixed run is 129 bytes */
    APPEND_MIXED_BYTES;
  }
  unsigned rle_len = rle_next - rle_buffer_next - 3;
  /* Store rle line meta data (length and (non)zero status) */
  if (nonzero) { /* Check for nonempty (no black pixels) line */
    if (job_options->ql_series) {
      rle_buffer_next [0] = 'g';
      rle_buffer_next [1] = (rle_len >> 8) & 0xff;
      rle_buffer_next [2] =  rle_len       & 0xff;
    } else {
      rle_buffer_next [0] = 'G';
      rle_buffer_next [1] =  rle_len       & 0xff;
      rle_buffer_next [2] = (rle_len >> 8) & 0xff;
    }
    rle_buffer_next = rle_next;
  } else {
    rle_buffer_next [0] = 'Z';
    rle_buffer_next++;
  }
  lines_waiting++;
  if (lines_waiting >= max_lines_waiting)
    flush_rle_buffer (job_options, header);
}

/**
 * Store a number of empty lines in rle_buffer using RLE.
 * @param job_options     Job options
 * @param header          Page header
 * @param empty_lines     Number of empty lines to store
 * @param xormask         The XOR mask for negative printing
 */
static inline void
RLE_store_empty_lines (job_options_t* job_options,
                       cups_page_header2_t* header,
                       int empty_lines,
                       unsigned char xormask) {
  int bytes_per_line = job_options->bytes_per_line;
  lines_waiting += empty_lines;
  if (xormask) {
    int blocks = (bytes_per_line + 127) / 128;
    ensure_rle_buf_space (job_options, header,
                          empty_lines * blocks);

    for (; empty_lines--; ) {
      /* leave space for the command prefix */
      rle_buffer_next += 3;
      unsigned char *start = rle_buffer_next;
      int len, rep_len;
      for (len = bytes_per_line; len > 0; len -= rep_len) {
        rep_len = len;
        if (rep_len > 129) rep_len = 129;
        *(rle_buffer_next++) = (signed char) (1 - rep_len);
        *(rle_buffer_next++) = xormask;
      }
      unsigned int rle_len = rle_buffer_next - start;
      if (job_options->ql_series) {
	start [-3] = 'g';
	start [-2] = (rle_len >> 8) & 0xff;
	start [-1] = rle_len & 0xff;
      } else {
	start [-3] = 'G';
	start [-2] = rle_len & 0xff;
	start [-1] = (rle_len >> 8) & 0xff;
      }
    }
  } else {
    ensure_rle_buf_space (job_options, header, empty_lines);
    for (; empty_lines--; ) *(rle_buffer_next++) = 'Z';
  }
}

/**
 * Emit raster lines for current page.
 * @param job_options   Job options
 * @param ras           Raster data stream
 * @param header        Page header
 * @return              0 on success, nonzero otherwise
 */
int
emit_raster_lines (job_options_t* job_options,
                   cups_raster_t* ras,
                   cups_page_header2_t* header) {
  unsigned char xormask = (header->NegativePrint ? ~0 : 0);
  /* Determine whether we need to mirror the pixel data */
  int do_mirror = job_options->software_mirror && job_options->mirror_print;

  unsigned cupsBytesPerLine = header->cupsBytesPerLine;
  unsigned cupsHeight = header->cupsHeight;
  unsigned cupsWidth = header->cupsWidth;
  int bytes_per_line = job_options->bytes_per_line;
  unsigned buflen = cupsBytesPerLine;
  /* Make sure buflen can be written as a byte */
  if (buflen > 0xff) buflen = 0xff;
  /* Truncate buflen if greater than bytes_per_line */
  if (buflen >= bytes_per_line) buflen = bytes_per_line;
  /* Calculate extra horizontal spacing pixels if the right side of */
  /* cupsImagingBBox doesn't touch the cupsPageSize box             */
  float pt2px[] = {
    header->HWResolution [0] / 72.0,
    header->HWResolution [1] / 72.0
  };
  unsigned right_spacing_px = 0;
  if (header->cupsImagingBBox[2] < header->cupsPageSize[0]) {
    right_spacing_px =
      (header->cupsPageSize[0] - header->cupsImagingBBox[2]) * pt2px [0];
  }
  /* Calculate right_padding_bytes and shift */
  unsigned right_padding_bits;
  if (job_options->align == CENTER) {
    unsigned left_spacing_px =
      header->cupsImagingBBox[0] * pt2px [0];
    unsigned width = left_spacing_px + cupsWidth + right_spacing_px;
    if (bytes_per_line * 8 < width)
      right_padding_bits = 0;
    else
      right_padding_bits = (bytes_per_line * 8 - width) / 2 + right_spacing_px;
  } else
    right_padding_bits = right_spacing_px;
  int right_padding_bytes = right_padding_bits / 8;
  int shift = right_padding_bits % 8;
  /* If width is not an integral number of bytes, we must shift   */
  /* right if we don't mirror, to ensure printing starts leftmost */
  if (!do_mirror) shift -= (8 - cupsWidth % 8) % 8;
  int shift_positive = (shift > 0 ? 1 : 0);
  /* We cannot allow buffer+padding to exceed device width */
  if (buflen + right_padding_bytes + shift_positive > bytes_per_line) {
    /* We cannot allow padding to exceed device width */
    if (right_padding_bytes + shift_positive > bytes_per_line)
      right_padding_bytes = bytes_per_line - shift_positive;
    /* Truncate buffer to fit device width */
    buflen = bytes_per_line - right_padding_bytes - shift_positive;
  }
  /* Generate and store empty lines if the top of cupsImagingBBox */
  /* doesn't touch the cupsPageSize box                           */
  unsigned top_empty_lines = 0;
  float page_size_y = header->cupsPageSize [1];
  if (header->cupsImagingBBox [3] != 0
      && (!job_options->concat_pages || job_options->page == 1)) {
    float top_distance_pt
      = page_size_y - header->cupsImagingBBox [3];
    top_empty_lines = lrint (top_distance_pt * pt2px [1]);
  }

  unsigned image_height_px = lrint (page_size_y * pt2px [1]);
  unsigned bot_empty_lines = 0;
  if (image_height_px >= top_empty_lines + cupsHeight)
    bot_empty_lines = image_height_px - top_empty_lines - cupsHeight;

  /*
   * QL printers have a specific top and bottom margin that must be left blank
   * to allow printers to skip to the next label.  For continuous-length tape,
   * this margin is defined as the minimum value allowed for the "ESC i d"
   * command as defined in the manuals; if a smaller value is passed, the
   * printer rounds that up.  For die-cut labels, the margin is implicit (the
   * "ESC i d" command is always passed a value of 0).  For most die-cut
   * labels, the margin is the same as the minimum continuous-length tape
   * margin, but there are a few exceptions.
   *
   * PT printers only support continuous-length tape.  They are documented to
   * work like QL printers, but in practice, they all seem to allow a minimum
   * margin of 0 as well.  The minimum margin still makes sure the tape is cut
   * in a blank space, so use the minimum margin amounts on those printers for
   * which we have a specification.
   *
   * Below, we ensure that printers for which a min_margin is defined will
   * always have a margin at lest that wide for continuous-length tape.  For
   * die-cut labels, we assume that the page margins are equal to the implicit
   * margins.  For page margins that are empty, we assume min_margin; in that
   * case, we'll end up skipping lines at the beginning and/or end of the
   * bitmap to allow for that minimum margin.
   */
  unsigned top_skip = 0, bot_skip = 0;
  unsigned min_feed = lrint (job_options->min_margin * pt2px [1]);
  if (job_options->media == LABELS && top_empty_lines) {
    top_empty_lines = 0;
  } else if (top_empty_lines >= min_feed) {
    top_empty_lines -= min_feed;
  } else {
    top_skip = min_feed - top_empty_lines;
    top_empty_lines = 0;
  }
  if (job_options->media == LABELS && bot_empty_lines) {
    bot_empty_lines = 0;
  } else if (bot_empty_lines >= min_feed) {
    bot_empty_lines -= min_feed;
  } else {
    bot_skip = min_feed - bot_empty_lines;
    bot_empty_lines = 0;
  }

  progress.page = job_options->page;
  progress.height = cupsHeight;

  /* Generate and store actual page data */
  empty_lines += top_empty_lines;
  int y;
  for (y = 0; y < cupsHeight; y++) {
    /* Feedback to the user */
    progress.completed = y;
    /* Read one line of pixels */
    if (cupsRasterReadPixels (ras, buffer, cupsBytesPerLine) < 1)
      break;  /* Escape if no pixels read */
    if (y < top_skip || y + bot_skip >= cupsHeight)
      continue;
    bool nonempty_line =
      generate_emit_line (buffer, emit_line_buffer, buflen, bytes_per_line,
                          right_padding_bytes, shift, do_mirror, xormask);
    if (nonempty_line) {
      if (empty_lines) {
        RLE_store_empty_lines
          (job_options, header, empty_lines, xormask);
        empty_lines = 0;
      }
      RLE_store_line (job_options, header,
                      emit_line_buffer, bytes_per_line);
    } else
      empty_lines++;
  }
  progress.completed = cupsHeight;
  report_progress (0);

  if (bot_empty_lines != 0 && !job_options->concat_pages)
    empty_lines += bot_empty_lines;
  return 0;
}

/**
 * Process CUPS raster data from input file, emitting printer data on
 * stdout.
 * @param job_options  Pointer to print options
 */
void
process_rasterdata (job_options_t* job_options) {
  cups_raster_t* ras;              /* Raster stream for printing    */
  cups_page_header2_t headers[2];  /* Page headers                  */
  int bytes_per_line = job_options->bytes_per_line;
  cups_page_header2_t *next_header = headers;
  cups_page_header2_t *header = headers + 1;
  cups_page_header2_t *tmp_header;

  ras = cupsRasterOpen (0, CUPS_RASTER_READ);
  for (job_options->page = 1,
         job_options->last_page = ! cupsRasterReadHeader2 (ras, header);
       ! job_options->last_page;
       tmp_header = next_header,
         next_header = header,
         header = tmp_header,
	 job_options->page++) {
    float pt2px[] = {
      header->HWResolution [0] / 72.0,
      header->HWResolution [1] / 72.0
    };
    fprintf (stderr, "DEBUG: %s: PageSize: %.2fx%.2f pt / %.2fx%.2f mm / %.2fx%.2f px\n",
	     progname,
	     header->cupsPageSize [0], header->cupsPageSize [1],
	     header->cupsPageSize [0] * MM_PER_PT, header->cupsPageSize [1] * MM_PER_PT,
	     header->cupsPageSize [0] * pt2px [0], header->cupsPageSize [1] * pt2px [1]);
    float *bbox = header->cupsImagingBBox;
    fprintf (stderr, "DEBUG: %s: ImagingBoundingBox: %.2f %.2f %.2f %.2f pt / "
	     "%.2f %.2f %.2f %.2f mm /"
	     "%.2f %.2f %.2f %.2f px\n",
	     progname,
	     bbox [0], bbox [1], bbox [2], bbox [3],
	     bbox [0] * MM_PER_PT, bbox [1] * MM_PER_PT,
	     bbox [2] * MM_PER_PT, bbox [3] * MM_PER_PT,
	     bbox [0] * pt2px [0], bbox [1] * pt2px [1],
	     bbox [2] * pt2px [0], bbox [3] * pt2px [1]);
    fprintf (stderr, "DEBUG: %s: HWResolution: %dx%ddpi\n",
	     progname,
	     header->HWResolution [0], header->HWResolution [1]);
    fprintf (stderr, "DEBUG: %s: Width Height: %u %u\n",
	     progname,
	     header->cupsWidth, header->cupsHeight);
    fprintf (stderr, "DEBUG: %s: NegativePrint: %d\n",
	     progname, header->NegativePrint);
    page_prepare (header->cupsBytesPerLine, bytes_per_line);
    if (job_options->page == 1) {
      emit_job_cmds (job_options);
      emit_page_cmds (job_options, header);
    }
    emit_raster_lines (job_options, ras, header);
    unsigned char xormask = (header->NegativePrint ? ~0 : 0);
    /* Determine whether this is the last page (fetch next)    */
    job_options->last_page = ! cupsRasterReadHeader2 (ras, next_header);
    if (! job_options->last_page) {
      if (!job_options->concat_pages) {
        RLE_store_empty_lines
          (job_options, header, empty_lines, xormask);
        empty_lines = 0;
        flush_rle_buffer (job_options, header);
	/* Emit page end marker without feed */
        putchar (PTC_FORMFEED);
      }
    } else {
      if (!job_options->concat_pages) {
        RLE_store_empty_lines
          (job_options, header, empty_lines, xormask);
        empty_lines = 0;
        flush_rle_buffer (job_options, header);
      } else {
        unsigned bot_empty_lines
          = lrint (header->cupsImagingBBox [1] * pt2px [1]);
        empty_lines = bot_empty_lines;
        RLE_store_empty_lines
          (job_options, header, empty_lines, xormask);
        empty_lines = 0;
        flush_rle_buffer (job_options, header);
      }
      /* Emit end-of-job command */
      putchar (PTC_EJECT);
    }
    page_end ();
    /* Emit page count according to CUPS requirements */
    fprintf (stderr, "PAGE: %d 1\n", job_options->page);
  }
}

static void help (void) {
  printf ("Usage: %s [options] {job-options}\n"
	  "\n"
	  "Options:\n"
	  "  -i, --input=NAME   read from NAME instead of standard input\n"
	  "  -o, --output=NAME  write to NAME instead of standard output\n"
	  "  -h, --help         display this help and exit\n",
	  progname);
}

static void fail_bad_options () {
  fprintf (stderr, "Try '%s --help' for more information\n", progname);
  exit (2);
}

/**
 * Main entry function.
 * @param argc  number of command line arguments plus one
 * @param argv  command line arguments
 * @return      0 if success, nonzero otherwise
 */
int
main (int argc, char* argv []) {
  progname = basename (argv[0]);
  const char *input_filename = NULL;
  const char *output_filename = NULL;

  for (;;) {
    static struct option long_options[] = {
      { "input",  1, NULL, 'i' },
      { "output",  1, NULL, 'o' },
      { "help",   0, NULL, 'h' },
      { }
    };

    int c = getopt_long (argc, argv, "hi:o:", long_options, NULL);
    if (c == -1)
      break;

    switch (c) {
    case 'h':  /* --help */
      help();
      exit(0);

    case 'i':  /* --input=NAME */
      input_filename = optarg;
      break;

    case 'o':  /*  --output=NAME */
      output_filename = optarg;
      break;

    case '?':  /* unknown option or missing argument */
      fail_bad_options ();
    }
  }

  if (optind >= argc) {
    fprintf (stderr, "%s: {job-options} argument missing\n", progname);
    fail_bad_options ();
  }

  job_options_t job_options = parse_job_options (argv [optind]);

  fprintf(stderr, "DEBUG: %s: job options: %s\n", progname, argv [optind]);

  if (input_filename) {
    int fd = open (input_filename, O_RDONLY);
    if (fd < 0) {
      fprintf (stderr, "%s: %s: %s\n", progname, input_filename, strerror(errno));
      exit (1);
    }
    dup2 (fd, 0);
    close (fd);
  }

  if (output_filename) {
    int fd = open (output_filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0) {
      fprintf (stderr, "%s: %s: %s\n", progname, output_filename, strerror(errno));
      exit (1);
    }
    dup2 (fd, 1);
    close (fd);
  }

  signal (SIGALRM, report_progress);
  struct itimerval it = { };
  it.it_value.tv_sec = 1;
  it.it_interval.tv_sec = 1;
  setitimer (ITIMER_REAL, &it, NULL);

  process_rasterdata (&job_options);

  return 0;
}
