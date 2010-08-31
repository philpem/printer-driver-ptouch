import base64
import sys
import os
import fnmatch
import cPickle
import gzip
import logging
from random import randint
import compressor
import ppd

def archive(ppds_directory):
    """Returns a string with the decompressor, its dependencies and the archive.

    It reads the template at pyppd/pyppd-ppdfile.in, inserts the dependencies
    and the archive encoded in base64, and returns as a string.

    """
    logging.info('Compressing folder "%s".' % ppds_directory)
    ppds_compressed = compress(ppds_directory)
    if not ppds_compressed:
        return None

    ppds_compressed_b64 = base64.b64encode(ppds_compressed)

    logging.info('Populating template.')
    template = read_file_in_syspath("pyppd/pyppd-ppdfile.in")
    compressor_py = read_file_in_syspath("pyppd/compressor.py")

    template = template.replace("@compressor@", compressor_py)
    template = template.replace("@ppds_compressed_b64@", ppds_compressed_b64)

    return template

def compress(directory):
    """Compresses and indexes *.ppd and *.ppd.gz in directory returning a string.

    The directory is walked recursively, concatenating all ppds found in a string.
    For each, it tests if its filename ends in *.gz. If so, opens with gzip. If
    not, opens directly. Then, it parses and saves its name, description (in the
    format CUPS needs (which can be more than one)) and it's position in the ppds
    string (start position and length) into a dictionary, used as an index.
    Then, it compresses the string, adds into the dictionary as key ARCHIVE and
    returns a compressed pickle dump of it.

    """
    ppds = ""
    ppds_index = {}
    abs_directory = os.path.abspath(directory)

    for ppd_path in find_files(directory, ("*.ppd", "*.ppd.gz")):
        # Remove 'directory/' from the filename
        ppd_filename = ppd_path[len(abs_directory)+1:]

        if ppd_path.lower().endswith(".gz"):
            ppd_file = gzip.open(ppd_path).read()
            # We don't want the .gz extension in our filename
            ppd_filename = ppd_filename[:-3]
        else:
            ppd_file = open(ppd_path).read()

        start = len(ppds)
        length = len(ppd_file)
        logging.debug('Found %s (%d bytes).' % (ppd_path, length))

        ppd_parsed = ppd.parse(ppd_file, ppd_filename)
        ppd_descriptions = map(str, ppd_parsed)
        ppds_index[ppd_parsed[0].uri] = (start, length, ppd_descriptions)
        logging.debug('Adding %d entry(ies): %s.' % (len(ppd_descriptions), map(str, ppd_parsed)))
        ppds += ppd_file

    if not ppds:
        logging.error('No PPDs found in folder "%s".' % directory)
        return None

    logging.info('Compressing archive.')
    ppds_index['ARCHIVE'] = compressor.compress(ppds)
    logging.info('Generating and compressing pickle dump.')
    ppds_pickle = compressor.compress(cPickle.dumps(ppds_index))


    return ppds_pickle


def read_file_in_syspath(filename):
    """Reads the file in filename in each sys.path.

    If we couldn't find, throws the last IOError caught.

    """
    last_exception = None
    for path in sys.path:
        try:
            return open(path + "/" + filename).read()
        except IOError as ex:
            last_exception = ex
            continue
    raise last_exception

def find_files(directory, patterns):
    """Yields each file that matches any of patterns in directory."""
    logging.debug('Searching for "%s" files in folder "%s".' %
                  (", ".join(patterns), directory))
    abs_directory = os.path.abspath(directory)
    for root, dirnames, filenames in os.walk(abs_directory):
        for pattern in patterns:
            for filename in fnmatch.filter(filenames, pattern):
                yield os.path.join(root, filename)
