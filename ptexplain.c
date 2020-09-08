#define _GNU_SOURCE
#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <getopt.h>

#if HAVE_LIBPNG
# include <png.h>
#endif

#define array_size(a) (sizeof (a) / sizeof (a)[0])

#define ESC 0x1b
#define CTRL_Z 0x1a

#define COMMAND_STACK_SIZE 128
#define HEX_CHUNK_SIZE 16

#if COMMAND_STACK_SIZE < HEX_CHUNK_SIZE
#error COMMAND_STACK_SIZE must be >= HEX_CHUNK_SIZE
#endif

bool silent;
bool verbose;
const char *write_prefix;
unsigned int noisy_commands_ignored;

enum data_type {
  INFO,
  CONTROL,
  DATA,
  RASTER,
  RUNLENGTH,
  FLAG_SET,
  FLAG_CLEARED,
  ERROR
};

struct color {
  const char *on;
  const char *off;
};
struct color colors[] = {
  { "", "" },
  { "\e[34m", "\e[0m" },
  { "\e[32m", "\e[0m" },
  { "\e[33m", "\e[0m" },
  { "\e[33;1m", "\e[0m" },
  { "\e[33;1m", "\e[0m" },
  { "\e[33m", "\e[0m" },
  { "\e[31;1m", "\e[0m" },
};

static void flush_silent_commands (void) {
  if (noisy_commands_ignored) {
    printf ("(%u commands hidden)\n",
	    noisy_commands_ignored);
    noisy_commands_ignored = 0;
  }
}

__attribute__ ((format (printf, 2, 3)))
static void print_message (enum data_type type, const char *fmt, ...)
{
  struct color *color = colors + type;
  va_list ap;

  flush_silent_commands ();
  va_start (ap, fmt);
  printf ("%s", color->on);
  vprintf (fmt, ap);
  printf ("%s\n", color->off);
  va_end (ap);
}

struct command {
  unsigned char type[COMMAND_STACK_SIZE];
  unsigned char data[COMMAND_STACK_SIZE];
  unsigned int len;
};

#define ERROR_FLAG 0x80

struct command command;

static void check_command_stack_overflow (void) {
  if (command.len >= array_size (command.data)) {
    print_message (ERROR, "Command stack overflow");
    exit (1);
  }
}

static void push (enum data_type type, int c) {
  check_command_stack_overflow ();
  command.type[command.len] = type;
  command.data[command.len] = c;
  command.len++;
}

static void reset_command (void) {
  command.len = 0;
}

static void vprint_command (const char *fmt, va_list *ap) {
  bool first = false;
  int n;

  if (command.len == 0)
    return;

  for (n = 0, first = true; n < command.len; n++, first = false) {
    struct color *color;

    if (command.type [n] & ERROR_FLAG)
      color = colors + ERROR;
    else
      color = colors + command.type [n];

    if ((command.type [n] & ~ERROR_FLAG) == CONTROL) {
      switch (command.data [n]) {
      case ESC:
        printf (" %sESC%s" + first,
		color->on, color->off);
	break;

      case CTRL_Z:
        printf (" %s^Z%s" + first,
		color->on, color->off);
	break;

      default:
        if (isprint (command.data [n]))
          printf (" %s%c%s" + first,
		  color->on, command.data [n], color->off);
	else
	  printf (" %s%02x%s" + first,
		  color->on, command.data [n], color->off);
	break;
      }
    } else {
      printf (" %s%02x%s" + first,
	      color->on, command.data [n], color->off);
    }
  }
  if (fmt) {
    printf(" ");
    vprintf (fmt, *ap);
  }
  printf ("\n");
  command.len = 0;
}

__attribute__ ((format (printf, 1, 2)))
static void print_command (const char *fmt, ...)
{
  va_list ap;

  flush_silent_commands ();
  va_start (ap, fmt);
  vprint_command (fmt, &ap);
  va_end (ap);
}

__attribute__ ((format (printf, 1, 2)))
static void print_noisy_command (const char *fmt, ...)
{
  va_list ap;

  if (silent) {
    noisy_commands_ignored++;
    reset_command ();
    return;
  }
  va_start (ap, fmt);
  vprint_command (fmt, &ap);
  va_end (ap);
}

static void die (void) {
  unsigned int lines = 5;

  for (;;) {
    int c = getchar ();
    if (command.len == HEX_CHUNK_SIZE) {
      char text[HEX_CHUNK_SIZE + 1];
      unsigned int n;

      for (n = 0; n < HEX_CHUNK_SIZE; n++) {
	text[n] = command.data[n];
	if (! isprint (text[n]))
	  text[n] = '.';
      }
      text[HEX_CHUNK_SIZE] = 0;

      print_command (" |%s|", text);
      reset_command ();

      if (! lines--) {
	if (c != EOF)
	  printf("...\n");
	break;
      }
    }
    if (c == EOF) {
      print_command (NULL);
      break;
    }
    push (DATA, c);
  }
  exit (1);
}

static int get (enum data_type type) {
  int c;

  c = getchar ();
  if (c == EOF) {
    if (command.len) {
      print_message (ERROR, "More data expected");
      die ();
    }
    return EOF;
  }
  push (type, c);
  return c;
}

static const unsigned char *get_more (enum data_type type, unsigned int n) {
  const unsigned char *start = command.data + command.len;
  while (n--)
    get (type);
  return start;
}

static void mark_error (unsigned int start, unsigned int len) {
  while (len-- != 0)
    command.type [start++] |= ERROR_FLAG;
}

static void unknown_command (void) {
  print_message (ERROR, "Unknown command");
  die ();
}

enum compression_mode {
  UNSPECIFIED,
  UNCOMPRESSED,
  TIFF
};

static void grow_buffer (unsigned char **buffer, unsigned long *reserved, unsigned long min_size) {
  unsigned char *new_buffer;
  unsigned int new_size;

  if (min_size <= *reserved)
    return;
  if (*reserved)
    new_size = *reserved << 1;
  else
    new_size = 64;
  while (new_size < min_size)
    new_size <<= 1;
  new_buffer = realloc (*buffer, new_size);
  if (! new_buffer) {
    fprintf(stderr, "Out of memory\n");
    exit (1);
  }
  *buffer = new_buffer;
  *reserved = new_size;
}

struct image {
  int row_size;
  unsigned char *buffer;
  unsigned long size;
  unsigned long reserved;
  unsigned int blank_rows;
};
struct image image = {
  -1,
};

static void add_row (const unsigned char *row, int row_size) {
  if (image.row_size == -1) {
    if (! row || row_size == -1) {
      image.blank_rows++;
      return;
    }
    image.row_size = row_size;
  }
  grow_buffer (&image.buffer, &image.reserved, image.size + image.row_size);
  if (! row || row_size == -1) {
    memset (image.buffer + image.size, 0, image.row_size);
  } else if (row_size == image.row_size) {
    memcpy (image.buffer + image.size, row, image.row_size);
  } else {
    static bool warned;

    if (!warned) {
      print_message (ERROR, "Row size changed from %u to %u bytes",
		     image.row_size, row_size);
      warned = true;
    }
    memset (image.buffer + image.size, 0, image.row_size);
  }
  image.size += image.row_size;
}

static void explain_raster_line (unsigned int bytes, enum compression_mode compression_mode) {
  if (compression_mode == TIFF) {
    static unsigned char *decompressed;
    static unsigned long reserved = 0;
    unsigned int row_size = 0, span;
    unsigned int n = 0;

    while (n < bytes) {
      int c = get (RUNLENGTH);
      n++;
      if ((signed char) c < 0) {
	if (n + 1 > bytes) {
	  mark_error (n - 1, 1);
	  die ();
	}
	span = 1 - (signed char) c;
	grow_buffer (&decompressed, &reserved, row_size + span);
	c = get (RASTER);
	n++;
	while (span--) {
	  decompressed [row_size] = c;
	  row_size++;
	}
      } else {
	span = c + 1;
	if (n + span > bytes) {
	  mark_error (n - 1, 1);
	  die ();
	}
	grow_buffer (&decompressed, &reserved, row_size + span);
	while (span--) {
	  decompressed [row_size] = get (RASTER);
	  row_size++;
	  n++;
	}
      }
    }
    if (write_prefix)
      add_row (decompressed, row_size);
    if (verbose)
      print_command ("(%d bytes)", row_size);
  } else {
    const unsigned char *d = get_more (RASTER, bytes);
    if (write_prefix)
      add_row (d, bytes);
    if (verbose)
      print_command (NULL);
  }
  reset_command ();
}

#if HAVE_LIBPNG
static char *next_filename (void) {
  static unsigned int number;
  char *filename;
  int len;

  number++;
  len = snprintf(NULL, 0, "%s%u.png", write_prefix, number);
  filename = malloc (len + 1);
  len = snprintf(filename, len + 1, "%s%u.png", write_prefix, number);
  return filename;
}

static void write_image (void)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_colorp palette;
  unsigned int pos;
  char *filename;
  FILE *fp;

  if (! write_prefix || ! image.size)
    return;

  filename = next_filename ();
  fp = fopen (filename, "wb");
  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info_ptr = png_create_info_struct (png_ptr);
  png_init_io (png_ptr, fp);
  png_set_IHDR (png_ptr, info_ptr,
	        /* width */ image.row_size * 8,
		/* height */ image.size / image.row_size + image.blank_rows,
	        /* bit depth */ 1,
		/* PNG_COLOR_TYPE_GRAY */ PNG_COLOR_TYPE_PALETTE,
		PNG_INTERLACE_NONE,
	        PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
  palette = (png_colorp) png_malloc(png_ptr, 2 * sizeof (png_color));
  palette[0].red = palette[0].green = palette[0].blue = 255;
  palette[1].red = palette[1].green = palette[1].blue = 0;
  png_set_PLTE(png_ptr, info_ptr, palette, 2);
  png_write_info (png_ptr, info_ptr);
  pos = image.size;
  while (pos > 0) {
    pos -= image.row_size;
    png_write_row (png_ptr, image.buffer + pos);
  }
  if (image.blank_rows) {
    unsigned char *blank_row = malloc (image.row_size);
    memset (blank_row, 0, image.row_size);
    while (image.blank_rows--)
      png_write_row (png_ptr, blank_row);
    image.blank_rows = 0;
  }
  png_write_end (png_ptr, NULL);
  png_destroy_write_struct (&png_ptr, &info_ptr);
  fclose (fp);

  print_message (RASTER, "Raster data written to %s", filename);
  free (filename);

  image.row_size = -1;
  image.size = 0;
}
#else /* HAVE_LIBPNG */
static void write_image (void)
{
}
#endif /* HAVE_LIBPNG */


static void check_compression_mode (enum compression_mode *compression_mode) {
  if (*compression_mode == UNSPECIFIED) {
    print_message (INFO, "Compression mode not specified; assuming no compression");
    *compression_mode = UNCOMPRESSED;
  }
}

struct flag {
  unsigned char mask;
  const char *name;
};

__attribute__ ((format (printf, 3, 4)))
static void sxprintf (char **str, char *end, const char *fmt, ...)
{
  va_list ap;
  int len;

  va_start (ap, fmt);
  len = vsnprintf (*str, end - *str - 1, fmt, ap);
  if (len > 0) {
    *str += len;
    **str = 0;
  }
  va_end (ap);
}

static const char *flags_str (unsigned char byte, const struct flag *flags)
{
  static struct color *color_set = colors + FLAG_SET;
  static struct color *color_cleared = colors + FLAG_CLEARED;
  static char buffer[256];

  const struct flag *flag;
  char *str, *end = buffer + sizeof(buffer);

  for (flag = flags, str = buffer; flag->mask; flag++) {
    if (byte & flag->mask) {
      sxprintf (&str, end, "%s%02x=%s%s ",
	color_set->on,
	flag->mask,
	flag->name,
	color_set->off);
    } else {
      sxprintf (&str, end, "%s%02x=%s%s ",
	color_cleared->on,
	flag->mask,
	flag->name,
	color_cleared->off);
    }
    byte &= ~flag->mask;
  }
  if (byte) {
    static struct color *color = colors + ERROR;
    sxprintf (&str, end, "%s%02x=%s%s ",
      color->on,
      byte,
      "unknown",
      color->off);
  }
  if (str == buffer)
    return "";
  str--;
  *str = 0;
  return buffer;
}

const struct flag print_information_valid_flags[] = {
  { 0x02, "kind" },
  { 0x04, "width" },
  { 0x08, "length" },
  { 0x40, "quality" }, /* not used on PT printers */
  { 0x80, "recover" },
  { }
};

const struct flag various_mode_flags[] = {
  /* { 0x1f, "feed" }, */  /* not officially documented */
  { 0x40, "auto_cut" },
  { 0x80, "mirror" },
  { }
};

const struct flag advanced_mode_flags[] = {
  { 0x01, "draft" },
  { 0x04, "half_cut" },
  { 0x08, "nochain" },
  { 0x10, "special_tape" },
  { 0x40, "hires" },
  { 0x80, "no_clearing" },
  { }
};

static void explain (void) {
  enum compression_mode compression_mode = UNSPECIFIED;
  bool initialized = false;
  int c;

  for (;;) {
    const char *what = "";
    const unsigned char *d;
    unsigned int u;

    c = get (CONTROL);
    if (c == EOF)
      break;

    if (c == 0) {
      int n = 1;
      while ((c = getchar ()) == 0)
	n++;
      print_command ("Reset (%u)", n);
      if (c == EOF)
	break;
      ungetc (c, stdin);
      initialized = false;
      continue;
    }

    if (c != ESC && ! initialized) {
      print_message (ERROR, "Initialize command missing");
      initialized = true;
    }

    switch (c) {
    case ESC:
      c = get (CONTROL);
      if (c != '@' && ! initialized) {
	print_message (ERROR, "Initialize command missing");
	initialized = true;
      }
      switch (c) {
      case '@':  /* ESC @ */
        print_command ("Initialize");
	initialized = true;
	break;

      case 'i':
	switch ((c = get (CONTROL))) {
	case '!':  /* ESC i ! */
	  /* QL-800/810W/820NWB, QL-1100/1110NWB/1115NWB */
	  c = get (DATA);
	  switch(c) {
	    case 0:
	      what = " (notify)";
	      break;
	    case 1:
	      what = " (do not notify)";
	      break;
	  }
	  print_command ("Switch automatic status notification mode%s", what);
	  break;

	case 'S':  /* ESC i S */
	  print_command ("Status information request");
	  break;

	case 'R':  /* ESC i R ## (legacy) */
	  what = " (legacy)";
	  /* fall through */

	case 'a':  /* ESC i a ## */
	  switch ((c = get (DATA))) {
	  case 0:
	    print_command ("Switch to ESC/P mode%s", what);
	    break;

	  case 1:
	    print_command ("Switch to raster mode%s", what);
	    break;

	  case 3:
	    print_command ("Switch to P-touch Template mode%s", what);
	    break;

	  default:
	    mark_error (3, 1);
	    print_command ("Switch to unknown mode");
	    break;
	  }
	  break;

	case 'z':  /* ESC i z */
	  d = get_more (DATA, 10);
	  what = flags_str (d[0], print_information_valid_flags);
	  char info[256];
	  info[0] = 0;
	  if (d[0] & 0x02)
	    sprintf(strchr(info, 0), " kind=0x%02x", d[1]);
	  if (d[0] & 0x04)
	    sprintf(strchr(info, 0), " width=%u", d[2]);
	  if (d[0] & 0x08)
	    sprintf(strchr(info, 0), " length=%u", d[3]);
	  sprintf(strchr(info, 0), " lines=%u",
	    (((((d[7] << 8) + d[6]) << 8) + d[5]) << 8) + d[4]);
	  switch(d[8]) {
	  case 0:
	    sprintf(strchr(info, 0), " page=first");
	    break;
	  case 1:
	    sprintf(strchr(info, 0), " page=non-first");
	    break;
	  case 2:
	    sprintf(strchr(info, 0), " page=last");
	    break;
	  }
	  print_command ("Print information command (%s)%s", what, info);
	  break;

	case 'M':  /* ESC i M */
	  c = get (DATA);
	  what = flags_str (c, various_mode_flags);
	  print_command ("Various mode settings%s%s%s",
			 *what ? " (" : "", what, *what ? ")" : "");
	  break;

	case 'K':  /* ESC i K */
	  c = get (DATA);
	  what = flags_str (c, advanced_mode_flags);
	  print_command ("Advanced mode settings%s%s%s",
			 *what ? " (" : "", what, *what ? ")" : "");
	  break;

	case 'd':  /* ESC i d */
	  d = get_more (DATA, 2);
	  u = d[0] + (d[1] << 8);
	  print_command ("Specify margin amount (%u lines)", u);
	  break;

	case 'U':  /* ESC i U */
	  /* Undocumented command used by the Windows 10 PT-P900W driver */
	  d = get_more (DATA, 15);
	  print_command ("Undocumented command");
	  break;

	case 'A':  /* ESC i A */
	  c = get (DATA);
	  print_command ("Cut every %u %s", c, c == 1 ? "label" : "labels");
	  break;

	case 'k':  /* ESC i k */
	  /* Undocumented command used by the Windows 10 PT-P900W driver */
	  d = get_more (DATA, 3);
	  print_command ("Undocumented command");
	  break;

	case 'c':  /* ESC i c */
	  d = get_more (DATA, 5);
	  print_command ("Legacy hires");
	  break;

	default:
	  unknown_command ();
	  break;
	}
	break;

      case EOF:
        break;

      default:
	unknown_command ();
      }
      break;

    case 'M':  /* M */
      c = get (DATA);
      switch (c) {
      case 0:
	compression_mode = UNCOMPRESSED;
        what = " (no compression)";
	break;
      case 2:
	compression_mode = TIFF;
        what = " (TIFF)";
	break;
      default:
        mark_error (1, 1);
      }
      print_command ("Select compression mode%s", what);
      break;

    case 'g':  /* g */
    case 'G':  /* G */
      d = get_more (DATA, 2);
      if (c == 'g') {
	if (d[0]) {
	  mark_error (1, 1);
	  die ();
	}
	u = d[1];
      } else
	u = d[0] + (d[1] << 8);
      check_compression_mode (&compression_mode);
      print_noisy_command ("Raster graphics transfer (%u bytes)", u);
      explain_raster_line (u, compression_mode);
      break;

    case 'Z':  /* Z */
      check_compression_mode (&compression_mode);
      if (compression_mode != TIFF) {
	static bool explained;

	if (! explained) {
	  what = " (not valid outside TIFF compression mode)";
	  explained = true;
	}
	mark_error (0, 1);
      }
      print_noisy_command ("Zero raster graphics%s", what);
      add_row (NULL, -1);
      break;

    case 0x0c:  /* Form Feed */
      print_command ("Print command");
      write_image ();
      break;

    case CTRL_Z: /* ^Z */
      print_command ("End of job");
      initialized = false;
      write_image ();
      break;

    default:
      unknown_command ();
      break;
    }
  }

  if (initialized)
    print_message (ERROR, "End of job command missing");
}

const char *progname;

static void usage (int status) {
  FILE *file = status ? stderr : stdout;

  fprintf (file,
	   "Usage: %s [OPTIONS]\n"
	   "Options are:\n"
	   "  -i, --input=NAME     file to read from (instead of standard input)\n"
#if HAVE_LIBPNG
	   "  -w, --write=PREFIX   write raster data to PREFIXn.png\n"
#endif
	   "  -s, --silent         hide raster graphics commands\n"
	   "  -v, --verbose        show all commands and all data\n"
	   "      --color={always,auto,never}\n"
	   "                       when to colorize the output\n"
	   "  -h, --help           this help\n",
	   progname);
  exit (status);
}

static struct option long_options[] = {
  { "input",       required_argument, NULL, 'i' },
  { "silent",      no_argument,       NULL, 's' },
  { "verbose",     no_argument,       NULL, 'v' },
#if HAVE_LIBPNG
  { "write",       required_argument, NULL, 'w' },
#endif
  { "color",       required_argument, NULL, 'c' },
  { "help",        no_argument,       NULL, 'h' },
  { }
};

int main(int argc, char *argv[]) {
  const char *filename = NULL;
  bool use_colors = isatty (1);
  const char *options;

  progname = basename (argv [0]);

  options = "w:i:svh" + (HAVE_LIBPNG ? 0 : 2);

  for (;;) {
    char c = getopt_long (argc, argv, options, long_options, NULL);
    if (c == -1)
      break;
    switch (c) {
      case 'i':  /* --input */
        filename = optarg;
        break;

      case 's':  /* --silent */
        silent = true;
	verbose = false;
	break;

      case 'v':  /* --verbose */
        verbose = true;
	silent = false;
	break;

      case 'w':  /* --write */
        write_prefix = optarg;
	break;

      case 'c':  /* --color */
	if (strcmp (optarg, "always") == 0)
	  use_colors = true;
	else if (strcmp (optarg, "auto") == 0)
	  use_colors = isatty (1);
	else if (strcmp (optarg, "never") == 0)
	  use_colors = false;
	else
	  usage (2);
	break;

      case 'h':  /* --help */
        usage (0);

      case '?':
        usage (2);
    }
  }

  if (filename) {
    int fd = open (filename, O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "%s: %s\n", filename, strerror (errno));
      exit (1);
    }
    dup2 (fd, 0);
  }

  if (! use_colors) {
    int n;
    for (n = 0; n < array_size (colors); n++) {
      colors[n].on = "";
      colors[n].off = "";
    }
    colors[FLAG_CLEARED].on = "[";
    colors[FLAG_CLEARED].off = "]";
  }

  explain ();
  return 0;
}
