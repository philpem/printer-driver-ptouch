pyppd
=====

``pyppd`` is a CUPS PPD generator. It holds an compressed archive of PPDs, which
can be listed and retrieved only when needed by CUPS, saving disk space.

Instalation
-----------

To install ``pyppd``, you can use:

  pip install pyppd

Or download the source package, uncompress, and run as root:

  python setup.py install

It depends on Python 2.x (http://www.python.org) and XZ Utils
(http://tukaani.org/xz/).

Usage
-----

At first, you have to create a PPD archive. For such, put all PPDs (they might
be gzipped) you want to add in the archive inside a single folder (which can
have subfolders), then run:

  pyppd /path/to/your/ppd/folder

It'll create ``pyppd-ppdfile`` in your current folder. You can test it by
running:

  ./pyppd-ppdfile list

And, for reading a PPD from the archive, simply do:

  ./pyppd-ppdfile cat pyppd-ppdfile:MY-PPD-FILE.PPD

For CUPS to be able to use your newly-created archive, copy ``pyppd-ppdfile``
to ``/usr/lib/cups/driver/`` and you're done.
