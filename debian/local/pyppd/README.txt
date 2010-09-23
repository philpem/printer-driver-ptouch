pyppd
=====

``pyppd`` is a CUPS PPD generator. It holds an compressed archive of PPDs, which
can be listed and retrieved only when needed by CUPS, saving disk space.

Instalation
-----------

To install ``pyppd``, you can use:

  # pip install pyppd

Or download the source package, uncompress, and run as root:

  # python setup.py install

It depends on Python 2.x (http://www.python.org) and XZ Utils
(http://tukaani.org/xz/).

Usage
-----

At first, you have to create a PPD archive. For such, put all PPDs (they might
be gzipped) you want to add in the archive inside a single folder (which can
have subfolders), then run:

  $ pyppd /path/to/your/ppd/folder

It'll create ``pyppd-ppdfile`` in your current folder. You can test it by
running:

  $ ./pyppd-ppdfile list

And, for reading a PPD from the archive, simply do:

  $ ./pyppd-ppdfile cat pyppd-ppdfile:MY-PPD-FILE.PPD

For CUPS to be able to use your newly-created archive, copy ``pyppd-ppdfile``
to ``/usr/lib/cups/driver/`` and you're done.

The generated ``pyppd-ppdfile`` can be arbitrarily renamed, so that more than
one packed repository can be installed on one system. This can be useful if
you need a better performance, be it in time or memory usage. Note that also
the PPD URIs will follow the new name:

  $ ./pyppd-ppdfile list

  pyppd-ppdfile:LasterStar/LaserStar-XX100.ppd

  $ mv pyppd-ppdfile laserstar

  $ ./laserstar list

  laserstar:LaserStar/LaserStar-XX100.ppd

Contributors
------------

* **Till Kamppeter** - Original idea, mentoring and feedback. User #0.

* **Hin-Tak Leung** - Lots of technical suggestions.

* **Flávio Ribeiro** and **Diógenes Fernandes** - Refactorings and general Python's best practices tips.

* **Google's OSPO** - Initial funding at GSoC 2010.
