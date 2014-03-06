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
 *         rastertoptch job user title copies options [filename]
 *
 * @param printer  The name of the printer queue (ignored)
 * @param job      The numeric job ID (ignored)
 * @param user     The string from the originating-user-name (ignored)
 * @param title    The string from the job-name attribute (ignored)
 * @param copies   The number of copies to be printed (ignored)
 * @param options  String representations of the job template
 *                 parameters, separated by spaces. Boolean attributes
 *                 are provided as "name" for true values and "noname"
 *                 for false values. All other attributes are provided
 *                 as "name=value" for single-valued attributes and
 *                 "name=value1,value2,...,valueN" for set attributes
 * @param filename The request file (if omitted, read from stdin)
 *
 * Available options (default values in [brackets]):
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
 * @param PrintDensity=1|...|5   Print density level: 1=light, 5=dark
 * @param ConcatPages            Output all pages in one page [noConcatPages]
 * @param RLEMemMax              Maximum memory used for RLE buffer [1000000]
 * @param SoftwareMirror         Make the filter mirror pixel data
 *                               if MirrorPrint is requested [noSoftwareMirror]
 * @param LabelPreamble          Emit preamble containing print quality,
 *                               roll/label type, tape width, label height,
 *                               and pixel lines [noLabelPreamble]
 * @param Debug                  Emit diagnostic output to stderr [noDebug]
 *                               (only if compiled with DEBUG set)
 *
 * Information about media type, resolution, mirror print, negative
 * print, cut media, advance distance (feed) is extracted from the
 * CUPS raster page headers given in the input stream.  The MediaType
 * page header field can be either "roll" or "labels" for continuous
 * tape or pre-cut labels, respectively.
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
 * <tr><td>ESC i R ## (1b 69 52 ##)</td>
 *     <td>Set transfer mode</td>
 *     <td>##: ?: 1=?</td></tr>
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
 *     <td>Set size of right(?) margin to N=#1+256*#2 pixels</td></tr>
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
 * <tr><td>QL-650TD <td>auto     <td>ULP<td>300<td>720<td>90<td>DK12-62mm
 * <tr><td>PT-PC    <td>auto     <td>BIP<td>180<td>128<td> 3<td>TZ6-24mm
 * <tr><td>PT-18R   <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-18mm
 * <tr><td>PT-550A  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-36mm
 * <tr><td>PT-1500PC<td>manual   <td>RLE<td>180<td>112<td>14<td>TZ6-24mm
 * <tr><td>PT-1950  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-18mm
 * <tr><td>PT-1950VP<td>auto     <td>RLE<td>180<td>112<td>14<td>TZ6-18mm
 * <tr><td>PT-1960  <td>auto     <td>RLE<td>180<td> 96<td>12<td>TZ6-18mm
 * <tr><td>PT-2300  <td>auto     <td>RLE<td>180<td>112<td>14<td>TZ6-18mm
 * <tr><td>PT-2420PC<td>manual   <td>RLE<td>180<td>128<td>16<td>TZ6-24mm
 * <tr><td>PT-2450DX<td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-24mm
 * <tr><td>PT-2500PC<td>auto     <td>RLE<td>180<td>128<td>16<td>TZ6-24mm
 * <tr><td>PT-2600  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ,AV6-24mm
 * <tr><td>PT-2610  <td>auto     <td>RLE<td>180<td>128<td>16<td>TZ,AV6-24mm
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
 */
/** Default pixel transfer method */
#define PIXEL_XFER_DEFAULT         RLE
/** Default print quality */
#define PRINT_QUALITY_HIGH_DEFAULT true
/** Default half cut mode */
#define HALF_CUT_DEFAULT           false
/** Maximum number of bytes per line */
#define BYTES_PER_LINE_MAX         255 /* cf. ULP_emit_line */
/** Default number of bytes per line */
#define BYTES_PER_LINE_DEFAULT     90
/** Default pixel data alignment on narrow tapes */
#define ALIGN_DEFAULT              RIGHT
/** Maximum print density value */
#define PRINT_DENSITY_MAX          5
/** Default print density value (1: light, ..., 5:dark, 0: no change) */
#define PRINT_DENSITY_DEFAULT      0
/** Transfer mode default ??? (-1 = don't set) */
#define TRANSFER_MODE_DEFAULT      -1
/** Driver pixel data mirroring default */
#define SOFTWARE_MIRROR_DEFAULT    false
/** Label preamble emitting default */
#define LABEL_PREAMBLE_DEFAULT     false
/** Interlabel margin removal default */
#define CONCAT_PAGES_DEFAULT       false
/** RLE buffer maximum memory usage */
#define RLE_ALLOC_MAX_DEFAULT      1000000
/** Mirror printing default */
#define MIRROR_DEFAULT             false
/** Negative printing default */
#define NEGATIVE_DEFAULT           false
/** Cut media mode default */
#define CUT_MEDIA_DEFAULT          CUPS_CUT_NONE
/** Roll fed media default */
#define ROLL_FED_MEDIA_DEFAULT     true
/** Device resolution default in DPI */
#define RESOLUTION_DEFAULT         { 300, 300 }
/** Page size default in PostScript points */
#define PAGE_SIZE_DEFAULT          { 176, 142 } /* 62x50mm */
/** Image size default in pixels */
#define IMAGE_HEIGHT_DEFAULT       0
/** Feed amount default */
#define FEED_DEFAULT               0
/** When to perform feed default */
#define PERFORM_FEED_DEFAULT       CUPS_ADVANCE_NONE

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
#include <cups/raster.h>
#include <cups/cups.h>

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
# endif
#endif
#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
# if ! HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
typedef unsigned char _Bool;
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif



#ifdef DEBUG
#include <sys/times.h>
/** Debug flag */
int debug = 0;
/** Number of emitted lines */
unsigned emitted_lines = 0;
#endif

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

/** Flag signalling whether any errors were encountered. */
int error_occurred;

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

/** Macro for obtaining integer option values. */
#define OBTAIN_INT_OPTION(name, var, min, max)                  \
  cups_option                                                   \
    = cupsGetOption (name, num_options, cups_options);          \
  if (cups_option) {                                            \
    errno = 0;                                                  \
    char* rest;                                                 \
    long int var = strtol (cups_option, &rest, 0);              \
    if (errno || *rest != '\0' || rest == cups_option           \
        || var < min || var > max) {                            \
      fprintf (stderr, "ERROR: " name " '%s', "                 \
               "must be an integer N, where %ld <= N <= %ld\n", \
               cups_option, (long) min, (long) max);            \
      error_occurred = 1;                                       \
    } else                                                      \
      options.var = var;                                        \
  }

/** Macro for obtaining boolean option values. */
#define OBTAIN_BOOL_OPTION(name, var)                      \
  cups_option                                              \
    = cupsGetOption (name, num_options, cups_options);     \
  if (cups_option) options.var = true;                     \
  cups_option                                              \
    = cupsGetOption ("no"name, num_options, cups_options); \
  if (cups_option) options.var = false;                    \

/**
 * Struct type for holding all the job options.
 */
typedef struct {
  xfer_t pixel_xfer;    /**< pixel transfer mode                  */
  cups_bool_t print_quality_high; /**< print quality is high      */
  bool half_cut;        /**< half cut                             */
  int bytes_per_line;   /**< bytes per line (print head width)    */
  align_t align;        /**< pixel data alignment                 */
  int software_mirror;  /**< mirror pixel data if mirror printing */
  int print_density;    /**< printing density (0=don't change)    */
  int xfer_mode;        /**< transfer mode ???                    */
  int label_preamble;   /**< emit ESC i z ...                     */
  bool concat_pages;    /**< remove interlabel margins            */
  unsigned long rle_alloc_max; /**< max bytes used for rle_buffer */
} job_options_t;

/**
 * Struct type for holding current page options.
 */
typedef struct {
  cups_cut_t cut_media;    /**< cut media mode                     */
  cups_bool_t mirror;      /**< mirror printing                    */
  bool roll_fed_media;     /**< continuous (not labels) roll media */
  unsigned resolution [2]; /**< horiz & vertical resolution in DPI */
  unsigned page_size [2];  /**< width & height of page in points   */
  unsigned image_height;   /**< height of page image in pixels     */
  unsigned feed;           /**< feed size in points                */
  cups_adv_t perform_feed; /**< When to feed                       */
} page_options_t;

/**
 * Parse options given in command line argument 5.
 * @param argc  number of command line arguments plus one
 * @param argv  command line arguments
 * @return      options, where each option set to its default value if
 *              not specified in argv [5]
 */
job_options_t
parse_options (int argc, const char* argv []) {
  job_options_t options = {
    PIXEL_XFER_DEFAULT,
    PRINT_QUALITY_HIGH_DEFAULT,
    HALF_CUT_DEFAULT,
    BYTES_PER_LINE_DEFAULT,
    ALIGN_DEFAULT,
    SOFTWARE_MIRROR_DEFAULT,
    PRINT_DENSITY_DEFAULT,
    TRANSFER_MODE_DEFAULT,
    LABEL_PREAMBLE_DEFAULT,
    CONCAT_PAGES_DEFAULT,
    RLE_ALLOC_MAX_DEFAULT,
  };
  if (argc < 6) return options;
  int num_options = 0;
  cups_option_t* cups_options = NULL;
  num_options
    = cupsParseOptions (argv [5], num_options, &cups_options);
  const char* cups_option
    = cupsGetOption ("PixelXfer", num_options, cups_options);
  if (cups_option) {
    if (strcasecmp (cups_option, "ULP") == 0)
      options.pixel_xfer = ULP;
    else if (strcasecmp (cups_option, "RLE") == 0)
      options.pixel_xfer = RLE;
    else if (strcasecmp (cups_option, "BIP") == 0)
      options.pixel_xfer = BIP;
    else {
      fprintf (stderr, "ERROR: Unknown PicelXfer '%s', "
               "must be RLE, BIP or ULP\n", cups_option);
      error_occurred = 1;
    }
  }
  cups_option
    = cupsGetOption ("PrintQuality", num_options, cups_options);
  if (cups_option) {
    if (strcasecmp (cups_option, "High") == 0)
      options.print_quality_high = true;
    else if (strcasecmp (cups_option, "Fast") == 0)
      options.print_quality_high = false;
    else {
      fprintf (stderr, "ERROR: Unknown PrintQuality '%s', "
               "must be High or Fast\n", cups_option);
      error_occurred = 1;
    }
  }
  OBTAIN_BOOL_OPTION ("HalfCut", half_cut);
  OBTAIN_INT_OPTION ("BytesPerLine", bytes_per_line,
                     1, BYTES_PER_LINE_MAX);
  cups_option
    = cupsGetOption ("Align", num_options, cups_options);
  if (cups_option) {
    if (strcasecmp (cups_option, "Right") == 0)
      options.align = RIGHT;
    else if (strcasecmp (cups_option, "Center") == 0)
      options.align = CENTER;
    else {
      fprintf (stderr, "ERROR: Unknown Align '%s', "
               "must be Right or Center\n", cups_option);
      error_occurred = 1;
    }
  }
  OBTAIN_INT_OPTION ("PrintDensity", print_density,
                     0, PRINT_DENSITY_MAX);
  OBTAIN_BOOL_OPTION ("ConcatPages", concat_pages);
  OBTAIN_INT_OPTION ("RLEMemMax", rle_alloc_max, 0, LONG_MAX);
  OBTAIN_INT_OPTION ("TransferMode", xfer_mode, 0, 255);
  OBTAIN_BOOL_OPTION ("SoftwareMirror", software_mirror);
  OBTAIN_BOOL_OPTION ("LabelPreamble", label_preamble);
  /* Release memory allocated for CUPS options struct */
  cupsFreeOptions (num_options, cups_options);
  return options;
}

/**
 * Determine input stream and open it.  If there are 6 command line
 * arguments, argv[6] is taken to be the input file name
 * otherwise stdin is used.  This funtion exits the program on error.
 * @param argc  number of command line arguments plus one
 * @param argv  command line arguments
 * @return      file descriptor for the opened input stream
 */
int
open_input_file (int argc, const char* argv []) {
  int fd;
  if (argc == 7) {
    if ((fd = open (argv[6], O_RDONLY)) < 0) {
      perror ("ERROR: Unable to open raster file - ");
      sleep (1);
      exit (1);
    }
  } else
    fd = 0;
  return fd;
}

/**
 * Update page_options with information found in header.
 * @param header        CUPS page header
 * @param page_options  page options to be updated
 */
void
update_page_options (cups_page_header_t* header,
                     page_options_t* page_options) {
  page_options->cut_media = header->CutMedia;
  page_options->mirror = header->MirrorPrint;
  const char* media_type = header->MediaType;
  page_options->roll_fed_media /* Default is continuous roll */
    = (strcasecmp ("Labels", media_type) != 0);
  page_options->resolution [0] = header->HWResolution [0];
  page_options->resolution [1] = header->HWResolution [1];
  page_options->page_size [0] = header->PageSize [0];
  page_options->page_size [1] = header->PageSize [1];
  page_options->image_height = header->cupsHeight;
  page_options->feed = header->AdvanceDistance;
  page_options->perform_feed = header->AdvanceMedia;
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
  /* Initialise printer */
  putchar (ESC); putchar ('@');
  /* Emit print density selection command if required */
  int density = job_options->print_density;
  switch (density) {
  case 1: case 2: case 3: case 4: case 5:
    putchar (ESC); putchar ('i'); putchar ('D'); putchar (density);
    break;
  default: break;
  }
  /* Emit transfer mode selection command if required */
  int xfer_mode = job_options->xfer_mode;
  if (xfer_mode >= 0 && xfer_mode < 0x100) {
    putchar (ESC); putchar ('i'); putchar ('R'); putchar (xfer_mode);
  }
  /* Emit half cut selection command if required */
  if (job_options->half_cut) {
    putchar (ESC); putchar ('i'); putchar ('K'); putchar (0x04);
  }
}

/**
 * Emit feed, cut and mirror command codes.
 * @param do_feed    Emit codes to actually feed
 * @param feed       Feed size
 * @param do_cut     Emit codes to actually cut
 * @param do_mirror  Emit codes to mirror print
 */
inline void
emit_feed_cut_mirror (bool do_feed, unsigned feed,
                      bool do_cut,
                      bool do_mirror) {
  /* Determine feed nibble */
  unsigned feed_nibble;
  if (do_feed) {
    feed_nibble = lrint (feed / 2.6 + 2.4); /* one suggested conversion */
    if (feed_nibble > 31) feed_nibble = 31;
  } else
    feed_nibble = 0;
  /* Determine auto cut bit - we only handle after each page */
  unsigned char auto_cut_bit = do_cut ? 0x40 : 0x00;
  /* Determine mirror print bit*/
  unsigned char mirror_bit = do_mirror ? 0x80 : 0x00;
  /* Combine & emit printer command code */
  putchar (ESC); putchar ('i'); putchar ('M');
  putchar ((char) (feed & 0x1f) | auto_cut_bit | mirror_bit);
}

/**
 * Emit quality, roll fed media, and label size command codes.
 * @param job_options      Current job options
 * @param page_options     Current page options
 * @param page_size_y      Page size (height) in pt
 * @param image_height_px  Number of pixel lines in current page
 */
void
emit_quality_rollfed_size (job_options_t* job_options,
                           page_options_t* page_options,
                           unsigned page_size_y,
                           unsigned image_height_px) {
  bool roll_fed_media = page_options->roll_fed_media;
  /* Determine print quality bit */
  unsigned char print_quality_bit
    = (job_options->print_quality_high == CUPS_TRUE) ? 0x40 : 0x00;
  unsigned char roll_fed_media_bit = roll_fed_media ? 0x00 : 0x01;
  /* Get tape width in mm */
  int tape_width_mm = lrint (page_options->page_size [0] * MM_PER_PT);
  if (tape_width_mm > 0xff) {
    fprintf (stderr,
             "ERROR: Page width (%umm) exceeds 255mm\n",
             tape_width_mm);
    tape_width_mm = 0xff;
  }
  /* Get tape height in mm */
  unsigned tape_height_mm;
  if (roll_fed_media)
    tape_height_mm = 0;
  else
    tape_height_mm = lrint (page_size_y * MM_PER_PT);
  if (tape_height_mm > 0xff) {
    fprintf (stderr,
             "ERROR: Page height (%umm) exceeds 255mm; use continuous tape (MediaType=roll)\n",
             tape_height_mm);
    tape_height_mm = 0xff;
  }
  /* Combine & emit printer command code */
  putchar (ESC); putchar ('i'); putchar ('z');
  putchar (print_quality_bit); putchar (roll_fed_media_bit);
  putchar (tape_width_mm & 0xff); putchar (tape_height_mm & 0xff);
  putchar (image_height_px & 0xff);
  putchar ((image_height_px >> 8) & 0xff);
  putchar (0x00); putchar (0x00); putchar (0x00); putchar (0x00);
}
/**
 * Emit printer command codes at start of page for options that have
 * changed.
 * @param job_options       Job options
 * @param old_page_options  Page options for preceding page
 * @param new_page_options  Page options for page to be printed
 * @param force             Ignore old_page_options and emit commands
 *                          for selecting all options in new_page_options
 */
void
emit_page_cmds (job_options_t* job_options,
                page_options_t* old_page_options,
                page_options_t* new_page_options,
                bool force) {
  int tape_width_mm = -1;

  /* Set width and resolution */
  unsigned hres = new_page_options->resolution [0];
  unsigned vres = new_page_options->resolution [1];
  unsigned old_page_size_x = old_page_options->page_size [0];
  unsigned new_page_size_x = new_page_options->page_size [0];
  if (force
      || hres != old_page_options->resolution [0]
      || vres != old_page_options->resolution [1]
      || new_page_size_x != old_page_size_x)
    /* We only know how to select 360x360DPI or 360x720DPI */
    if (hres == 360 && (vres == 360 || vres == 720)) {
      /* Get tape width in mm */
      tape_width_mm = lrint (new_page_size_x * MM_PER_PT);
      if (tape_width_mm > 0xff) {
        fprintf (stderr,
                 "ERROR: Page width (%umm) exceeds 255mm\n",
                 tape_width_mm);
        tape_width_mm = 0xff;
      }
      /* Emit printer commands */
      putchar (ESC); putchar ('i'); putchar ('c');
      if (vres == 360) {
        putchar (0x84); putchar (0x00); putchar (tape_width_mm & 0xff);
        putchar (0x00); putchar (0x00);
      } else {
        putchar (0x86); putchar (0x09); putchar (tape_width_mm & 0xff);
        putchar (0x00); putchar (0x01);
      }
    }

  /* Set feed, auto cut and mirror print */
  unsigned feed = new_page_options->feed;
  cups_adv_t perform_feed = new_page_options->perform_feed;
  cups_cut_t cut_media = new_page_options->cut_media;
  cups_bool_t mirror = new_page_options->mirror;
  if (force
      || feed != old_page_options->feed
      || perform_feed != old_page_options->perform_feed
      || cut_media != old_page_options->cut_media
      || mirror != old_page_options->mirror)
    /* We only know how to feed after each page */
    emit_feed_cut_mirror (perform_feed == CUPS_ADVANCE_PAGE, feed,
                          cut_media == CUPS_CUT_PAGE,
                          mirror == CUPS_TRUE);
  /* Set media and quality if label preamble is requested */
  unsigned page_size_y = new_page_options->page_size [1];
  unsigned image_height_px = lrint (page_size_y * vres / 72.0);
  if (job_options->label_preamble && !job_options->concat_pages
      && (force
          || (new_page_options->roll_fed_media
              != old_page_options->roll_fed_media)
          || new_page_size_x != old_page_size_x
          || page_size_y != old_page_options->page_size [1]))
    emit_quality_rollfed_size (job_options, new_page_options,
                               page_size_y, image_height_px);

  /* WHY DON'T WE SET MARGIN (ESC i d ...)? */

  /* Set pixel data transfer compression */
  if (force) {
    if (job_options->pixel_xfer == RLE) {
      putchar ('M'); putchar (0x02);
    }
  }
  /* Emit number of raster lines to follow if using BIP */
  if (job_options->pixel_xfer == BIP) {
    unsigned image_height_px = lrint (page_size_y * vres / 72.0);
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
inline int
generate_emit_line (unsigned char* in_buffer,
                    unsigned char* out_buffer,
                    int buflen,
                    unsigned char bytes_per_line,
                    int right_padding_bytes,
                    int shift,
                    int do_mirror,
                    unsigned char xormask) {
#ifdef DEBUG
  if (debug)
    fprintf (stderr, "DEBUG: generate_emit_line "
             "(in_buffer=%0x, out_buffer=%0x, "
             "buflen=%d, bytes_per_line=%d, right_padding_bytes=%d, "
             "shift=%d, do_mirror=%d, xormask=%0x)\n",
             in_buffer, out_buffer, buflen, bytes_per_line,
             right_padding_bytes, shift, do_mirror, xormask);
#endif
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
 * @param page_options  Page options
 */
inline void
flush_rle_buffer (job_options_t* job_options,
                  page_options_t* page_options) {
#ifdef DEBUG
  if (debug)
    fprintf (stderr, "DEBUG: flush_rle_buffer (): "
             "lines_waiting = %d\n",
             lines_waiting);
#endif
  if (lines_waiting > 0) {
    if (job_options->label_preamble)
      emit_quality_rollfed_size (job_options, page_options,
                                 page_options->page_size [1],
                                 lines_waiting);
    xfer_t pixel_xfer = job_options->pixel_xfer;
    int bytes_per_line = job_options->bytes_per_line;
    switch (pixel_xfer) {
    case RLE: {
      size_t dummy
        = fwrite (rle_buffer, sizeof (char), rle_buffer_next - rle_buffer, stdout);
      break;
    }
    case ULP:
    case BIP: {
      unsigned char* p = rle_buffer;
      unsigned emitted_lines = 0;
      while (rle_buffer_next - p > 0) {
        if (pixel_xfer == ULP) {
          putchar ('g'); putchar (0x00); putchar (bytes_per_line);
        }
        int emitted = 0;
        int linelen;
        switch (*p++) {
        case 'G':
          linelen = *p++;
          linelen += ((int)(*p++)) << 8;
          while (linelen > 0) {
            signed char l = *p++; linelen--;
            if (l < 0) { /* emit repeated data */
              char data = *p++; linelen--;
              emitted -= l; emitted++;
              for (; l <= 0; l++) putchar (data);
            } else { /* emit the l + 1 following bytes of data */
              size_t dummy = fwrite (p, sizeof (char), l + 1, stdout);
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
#ifdef DEBUG
  if (debug)
    fprintf (stderr, "DEBUG: emitted %d lines\n", emitted_lines);
#endif
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
 * @param page_options  Page options
 * @param bytes         Number of bytes required.
 */
inline void
ensure_rle_buf_space (job_options_t* job_options,
                      page_options_t* page_options,
                      unsigned bytes) {
  unsigned long nextpos = rle_buffer_next - rle_buffer;
  if (nextpos + bytes > rle_alloced) {
    /* Exponential size increase avoids too frequent reallocation */
    unsigned long new_alloced = rle_alloced * 2 + 0x4000;
#ifdef DEBUG
  if (debug)
    fprintf (stderr, "DEBUG: ensure_rle_buf_space (bytes=%d): "
             "increasing rle_buffer from %d to %d\n",
             bytes,
             rle_alloced * sizeof (char),
             new_alloced * sizeof (char));
#endif
    void* p = NULL;
    if (new_alloced <= job_options->rle_alloc_max) {
      if (rle_buffer)
        p = (unsigned char*) realloc (rle_buffer, new_alloced * sizeof (char));
      else
        p = (unsigned char*) malloc (new_alloced * sizeof (char));
    }
    if (p) {
      rle_buffer = p;
      rle_buffer_next = rle_buffer + nextpos;
      rle_alloced = new_alloced;
    } else { /* Gain memory by flushing buffer to printer */
      flush_rle_buffer (job_options, page_options);
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
 * @param page_options  Page options
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
inline void
RLE_store_line (job_options_t* job_options,
                page_options_t* page_options,
                const unsigned char* buf, unsigned buf_len) {
  ensure_rle_buf_space (job_options, page_options,
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
    rle_buffer_next [0] = 'G';
    rle_buffer_next [1] =  rle_len       & 0xff;
    rle_buffer_next [2] = (rle_len >> 8) & 0xff;
    rle_buffer_next = rle_next;
  } else {
    rle_buffer_next [0] = 'Z';
    rle_buffer_next++;
  }
  lines_waiting++;
  if (lines_waiting >= max_lines_waiting)
    flush_rle_buffer (job_options, page_options);
}

/**
 * Store a number of empty lines in rle_buffer using RLE.
 * @param job_options     Job options
 * @param page_options    Page options
 * @param empty_lines     Number of empty lines to store
 * @param xormask         The XOR mask for negative printing
 */
inline void
RLE_store_empty_lines (job_options_t* job_options,
                       page_options_t* page_options,
                       int empty_lines,
                       unsigned char xormask) {
  int bytes_per_line = job_options->bytes_per_line;
#ifdef DEBUG
  if (debug)
    fprintf (stderr, "DEBUG: RLE_store_empty_lines (empty_lines=%d, "
             "bytes_per_line=%d): lines_waiting = %d\n",
             empty_lines, bytes_per_line, lines_waiting);
#endif
  lines_waiting += empty_lines;
  if (xormask) {
    int blocks = (bytes_per_line + 127) / 128;
    ensure_rle_buf_space (job_options, page_options,
                          empty_lines * blocks);
    for (; empty_lines--; ) {
      *(rle_buffer_next++) = 'G';
      *(rle_buffer_next++) = 0x02;
      *(rle_buffer_next++) = 0x00;
      int rep_len;
      for (; bytes_per_line > 0; bytes_per_line -= rep_len) {
        rep_len = bytes_per_line;
        if (rep_len > 128) rep_len = 128;
        *(rle_buffer_next++) = (signed char) (1 - rep_len);
        *(rle_buffer_next++) = xormask;
      }
    }
  } else {
    ensure_rle_buf_space (job_options, page_options, empty_lines);
    for (; empty_lines--; ) *(rle_buffer_next++) = 'Z';
  }
}

/**
 * Emit raster lines for current page.
 * @param page          Page number of page to be emitted
 * @param job_options   Job options
 * @param page_options  Page options
 * @param ras           Raster data stream
 * @param header        Current page header
 * @return              0 on success, nonzero otherwise
 */
int
emit_raster_lines (int page,
                   job_options_t* job_options,
                   page_options_t* page_options,
                   cups_raster_t* ras,
                   cups_page_header_t* header) {
  unsigned char xormask = (header->NegativePrint ? ~0 : 0);
  /* Determine whether we need to mirror the pixel data */
  int do_mirror = job_options->software_mirror && page_options->mirror;

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
  /* ImagingBoundingBox doesn't touch the PageSize box              */
  double scale_pt2xpixels = header->HWResolution [0] / 72.0;
  unsigned right_spacing_px = 0;
  if (header->ImagingBoundingBox [2] != 0) {
    unsigned right_distance_pt
      = header->PageSize [0] - header->ImagingBoundingBox [2];
    if (right_distance_pt != 0)
      right_spacing_px = right_distance_pt * scale_pt2xpixels;
  }
  /* Calculate right_padding_bytes and shift */
  int right_padding_bits;
  if (job_options->align == CENTER) {
    unsigned left_spacing_px = 0;
    if (header->ImagingBoundingBox [0] != 0)
      left_spacing_px
        = header->ImagingBoundingBox [0] * scale_pt2xpixels;
    right_padding_bits
      = (bytes_per_line * 8
         - (left_spacing_px + cupsWidth + right_spacing_px)) / 2
      + right_spacing_px;
    if (right_padding_bits < 0) right_padding_bits = 0;
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
#ifdef DEBUG
    if (debug) {
      fprintf (stderr, "DEBUG: Warning: buflen = %d, right_padding_bytes = %d, "
               "shift = %d, bytes_per_line = %d\n",
               buflen, right_padding_bytes, shift, bytes_per_line);
    }
#endif
    /* We cannot allow padding to exceed device width */
    if (right_padding_bytes + shift_positive > bytes_per_line)
      right_padding_bytes = bytes_per_line - shift_positive;
    /* Truncate buffer to fit device width */
    buflen = bytes_per_line - right_padding_bytes - shift_positive;
  }
  /* Percentage of page emitted */
  int completed = -1;
  /* Generate and store empty lines if the top of ImagingBoundingBox */
  /* doesn't touch the PageSize box                                  */
  double scale_pt2ypixels = header->HWResolution [1] / 72.0;
  unsigned top_empty_lines = 0;
  unsigned page_size_y = header->PageSize [1];
  if (header->ImagingBoundingBox [3] != 0
      && (!job_options->concat_pages || page == 1)) {
    unsigned top_distance_pt
      = page_size_y - header->ImagingBoundingBox [3];
    if (top_distance_pt != 0) {
      top_empty_lines = lrint (top_distance_pt * scale_pt2ypixels);
      empty_lines += top_empty_lines;
    }
  }
  /* Generate and store actual page data */
  int y;
  for (y = 0; y < cupsHeight; y++) {
    /* Feedback to the user */
    if ((y & 0x1f) == 0) {
      int now_completed = 100 * y / cupsHeight;
      if (now_completed > completed) {
        completed = now_completed;
        fprintf (stderr,
                 "INFO: Printing page %d, %d%% complete...\n",
                 page, completed);
        fflush (stderr);
      }
    }
    /* Read one line of pixels */
    if (cupsRasterReadPixels (ras, buffer, cupsBytesPerLine) < 1)
      break;  /* Escape if no pixels read */
    bool nonempty_line =
      generate_emit_line (buffer, emit_line_buffer, buflen, bytes_per_line,
                          right_padding_bytes, shift, do_mirror, xormask);
    if (nonempty_line) {
      if (empty_lines) {
        RLE_store_empty_lines
          (job_options, page_options, empty_lines, xormask);
        empty_lines = 0;
      }
      RLE_store_line (job_options, page_options,
                      emit_line_buffer, bytes_per_line);
    } else
      empty_lines++;
  }

  unsigned image_height_px = lrint (page_size_y * scale_pt2ypixels);
  unsigned bot_empty_lines;
  if (image_height_px >= top_empty_lines + y)
    bot_empty_lines = image_height_px - top_empty_lines - y;
  else
    bot_empty_lines = 0;
  if (bot_empty_lines != 0 && !job_options->concat_pages)
    empty_lines += bot_empty_lines;
  fprintf (stderr,
           "INFO: Printing page %d, 100%% complete.\n",
            page);
  fflush (stderr);
  return 0;
}
/**
 * Process CUPS raster data from input file, emitting printer data on
 * stdout.
 * @param fd           File descriptor for input file
 * @param job_options  Pointer to print options
 * @return             0 on success, nonzero otherwise
 */
int
process_rasterdata (int fd, job_options_t* job_options) {
  int page = 1;                    /* Page number                   */
  cups_raster_t* ras;              /* Raster stream for printing    */
  cups_page_header_t header;       /* Current page header           */
  int first_page = true;           /* Is this the first page?       */
  int more_pages;                  /* Are there more pages left?    */
  int bytes_per_line = job_options->bytes_per_line;
  page_options_t page_options [2] = {{
    CUT_MEDIA_DEFAULT,
    MIRROR_DEFAULT,
    ROLL_FED_MEDIA_DEFAULT,
    RESOLUTION_DEFAULT,
    PAGE_SIZE_DEFAULT,
    IMAGE_HEIGHT_DEFAULT,
    FEED_DEFAULT,
    PERFORM_FEED_DEFAULT,}
  };                               /* Current & preceding page opts */
  page_options_t* new_page_options
    = page_options + 0;            /* Options for current page      */
  page_options_t* old_page_options
    = page_options + 1;            /* Options for preceding page    */
  page_options_t* tmp_page_options;/* Temp variable for swapping    */
  ras = cupsRasterOpen (fd, CUPS_RASTER_READ);
  for (more_pages = cupsRasterReadHeader (ras, &header);
       more_pages;
       tmp_page_options = old_page_options,
         old_page_options = new_page_options,
         new_page_options = tmp_page_options,
         first_page = false) {
    update_page_options (&header, new_page_options);
#ifdef DEBUG
    if (debug) {
      fprintf (stderr, "DEBUG: pixel_xfer = %d\n", job_options->pixel_xfer);
      fprintf (stderr, "DEBUG: print_quality_high = %d\n", job_options->print_quality_high);
      fprintf (stderr, "DEBUG: half_cut = %d\n", job_options->half_cut);
      fprintf (stderr, "DEBUG: bytes_per_line = %d\n", job_options->bytes_per_line);
      fprintf (stderr, "DEBUG: align = %d\n", job_options->align);
      fprintf (stderr, "DEBUG: software_mirror = %d\n", job_options->software_mirror);
      fprintf (stderr, "DEBUG: label_preamble = %d\n", job_options->label_preamble);
      fprintf (stderr, "DEBUG: print_density = %d\n", job_options->print_density);
      fprintf (stderr, "DEBUG: xfer_mode = %d\n", job_options->xfer_mode);
      fprintf (stderr, "DEBUG: concat_pages = %d\n", job_options->concat_pages);
      fprintf (stderr, "DEBUG: cut_media = %d\n", new_page_options->cut_media);
      fprintf (stderr, "DEBUG: mirror = %d\n", new_page_options->mirror);
      fprintf (stderr, "DEBUG: roll_fed_media = %d\n", new_page_options->roll_fed_media);
      fprintf (stderr, "DEBUG: resolution = %d x %d\n", new_page_options->resolution [0], new_page_options->resolution [1]);
      fprintf (stderr, "DEBUG: page_size = %d x %d\n", new_page_options->page_size [0], new_page_options->page_size [1]);
      fprintf (stderr, "DEBUG: image_height = %d\n", new_page_options->image_height);
      fprintf (stderr, "DEBUG: feed = %d\n", new_page_options->feed);
      fprintf (stderr, "DEBUG: perform_feed = %d\n", new_page_options->perform_feed);
      fprintf (stderr, "DEBUG: header->ImagingBoundingBox = [%u, %u, %u, %u]\n",
               header.ImagingBoundingBox [0], header.ImagingBoundingBox [1],
               header.ImagingBoundingBox [2], header.ImagingBoundingBox [3]);
      fprintf (stderr, "DEBUG: header.Margins = [%u, %u]\n",
               header.Margins [0], header.Margins [1]);
    }
#endif
    page_prepare (header.cupsBytesPerLine, bytes_per_line);
    if (first_page) {
      emit_job_cmds (job_options);
      emit_page_cmds (job_options, old_page_options,
                      new_page_options, first_page);
    }
    emit_raster_lines (page, job_options, new_page_options, ras, &header);
    unsigned char xormask = (header.NegativePrint ? ~0 : 0);
    /* Determine whether this is the last page (fetch next)    */
    more_pages = cupsRasterReadHeader (ras, &header);
    /* Do feeding or ejecting at the end of each page. */
    cups_adv_t perform_feed = new_page_options->perform_feed;
    if (more_pages) {
      if (!job_options->concat_pages) {
        RLE_store_empty_lines
          (job_options, page_options, empty_lines, xormask);
        empty_lines = 0;
        flush_rle_buffer (job_options, page_options);
        if (perform_feed == CUPS_ADVANCE_PAGE)
          putchar (PTC_EJECT);    /* Emit eject marker to force feed   */
        else
          putchar (PTC_FORMFEED); /* Emit page end marker without feed */
      }
    } else {
      if (!job_options->concat_pages) {
        RLE_store_empty_lines
          (job_options, page_options, empty_lines, xormask);
        empty_lines = 0;
        flush_rle_buffer (job_options, page_options);
        putchar (PTC_FORMFEED);
      } else {
        double scale_pt2ypixels = header.HWResolution [1] / 72.0;
        unsigned bot_empty_lines
          = lrint (header.ImagingBoundingBox [1] * scale_pt2ypixels);
        empty_lines = bot_empty_lines;
        RLE_store_empty_lines
          (job_options, page_options, empty_lines, xormask);
        empty_lines = 0;
        flush_rle_buffer (job_options, page_options);
      }

      /* If special feed or cut at job end, emit commands to that effect */
      cups_cut_t cut_media = new_page_options->cut_media;
      if (perform_feed == CUPS_ADVANCE_JOB || cut_media == CUPS_CUT_JOB) {
        emit_feed_cut_mirror
          (perform_feed == CUPS_ADVANCE_PAGE ||
           perform_feed == CUPS_ADVANCE_JOB,
           new_page_options->feed,
           cut_media == CUPS_CUT_PAGE || cut_media == CUPS_CUT_JOB,
           new_page_options->mirror == CUPS_TRUE);
        /* Emit eject marker */
        putchar (PTC_EJECT);
      }
    }
    page_end ();
    /* Emit page count according to CUPS requirements */
    fprintf (stderr, "PAGE: %d 1\n", page);
    page++;
  }
  return 0;
}
/**
 * Main entry function.
 * @param argc  number of command line arguments plus one
 * @param argv  command line arguments
 * @return      0 if success, nonzero otherwise
 */
int
main (int argc, const char* argv []) {
  error_occurred = 0;
#ifdef DEBUG
  int i;
  if (argc > 5)
    if (strcasestr (argv [5], "debug") == argv [5]
        || strcasestr (argv [5], " debug") != NULL)
      debug = true;
  struct tms time_start, time_end;
  if (debug) {
    fprintf (stderr, "DEBUG: args = ");
    for (i = 0; i < argc; i++) fprintf (stderr, "%d:'%s' ", i, argv [i]);
    fprintf (stderr, "\nDEBUG: environment =\n");
    char** envvarbind;
    for (envvarbind = environ; *envvarbind; envvarbind++)
      fprintf (stderr, "DEBUG:  %s\n", *envvarbind);
    times (&time_start);
  }
#endif

  job_options_t job_options = parse_options (argc, argv);

  int fd = open_input_file (argc, argv);

  int rv = process_rasterdata (fd, &job_options);

#ifdef DEBUG
  if (debug) {
    times (&time_end);
    fprintf (stderr, "DEBUG: User time  System time  (usec)\n");
    fprintf (stderr, "DEBUG: %9.3g  %9.3g\n",
             (time_end.tms_utime - time_start.tms_utime)
             * 1000000.0 / CLOCKS_PER_SEC,
             (time_end.tms_stime - time_start.tms_stime)
             * 1000000.0 / CLOCKS_PER_SEC);
    fprintf (stderr, "DEBUG: Emitted lines: %u\n", emitted_lines);
  }
#endif

  if (fd != 0) close (fd);

  if (error_occurred) return error_occurred; else return rv;
}
