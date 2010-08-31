from subprocess import Popen, PIPE

def compress(value):
    """Compresses a string with the xz binary"""

    process = Popen(["xz", "--compress", "--force"], stdin=PIPE, stdout=PIPE)
    return process.communicate(value)[0]

def decompress(value):
    """Decompresses a string with the xz binary"""

    process = Popen(["xz", "--decompress", "--stdout", "--force"],
                    stdin=PIPE, stdout=PIPE)
    return process.communicate(value)[0]

def compress_file(path):
    """Compress the file at 'path' with the xz binary"""

    process = Popen(["xz", "--compress", "--force", "--stdout", path], stdout=PIPE)
    return process.communicate()[0]
