#!/usr/bin/env python

from distutils.core import setup
from distutils.command.sdist import sdist as _sdist

class sdist(_sdist):
    def run(self):
        try:
            import sys
            sys.path.append("contrib")
            import git2changes
            print 'generating CHANGES.txt'
            with open('CHANGES.txt', 'w+') as f:
                git2changes.run(f)
        except ImportError:
            pass
        
        _sdist.run(self)

setup(
    name='pyppd',
    version='0.4.3',
    author='Vitor Baptista',
    author_email='vitor@vitorbaptista.com',
    packages=['pyppd'],
    package_data={'pyppd': ['*.in']},
    scripts=['bin/pyppd'],
    url='http://gitorious.org/vitorbaptista/pyppd/',
    license='GPLv3',
    description='A CUPS PostScript Printer Driver\'s compressor and generator',
    long_description=open('README.txt').read(),
    cmdclass={'sdist': sdist},
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: System Administrators',
        'Operating System :: POSIX',
        'License :: OSI Approved :: GNU General Public License (GPL)',
        'Topic :: Printing',
        ],
)
