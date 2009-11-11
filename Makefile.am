## Process this file with automake to produce Makefile.in
## Copyright (c) 2006  Arne John Glenstrup <panic@itu.dk>
##
## This file is part of ptouch-driver.
##
## ptouch-driver is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or (at
## your option) any later version.
##
## ptouch-driver is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with ptouch-driver; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
## USA

execfilterdir			= $(libdir)/cups/filter
execfilter_PROGRAMS		= rastertoptch
rastertoptch_CPPFLAGS		= -D_GNU_SOURCE

foomaticdbdir			= $(datadir)/foomatic/db/source
nobase_dist_foomaticdb_DATA	= driver/ptouch.xml \
				  printer/Brother-QL-500.xml \
				  printer/Brother-QL-550.xml \
				  printer/Brother-QL-650TD.xml \
				  printer/Brother-PT-PC.xml \
				  printer/Brother-PT-18R.xml \
				  printer/Brother-PT-1500PC.xml \
				  printer/Brother-PT-1950VP.xml \
				  printer/Brother-PT-1950.xml \
				  printer/Brother-PT-1960.xml \
				  printer/Brother-PT-2300.xml \
				  printer/Brother-PT-2420PC.xml \
				  printer/Brother-PT-2450DX.xml \
				  printer/Brother-PT-2500PC.xml \
				  printer/Brother-PT-2600.xml \
				  printer/Brother-PT-2610.xml \
				  printer/Brother-PT-3600.xml \
				  printer/Brother-PT-550A.xml \
				  printer/Brother-PT-9200DX.xml \
				  printer/Brother-PT-9200PC.xml \
				  printer/Brother-PT-9400.xml \
				  printer/Brother-PT-9500PC.xml \
				  printer/Brother-PT-9600.xml \
				  opt/Brother-Ptouch-AdvanceDistance.xml \
				  opt/Brother-Ptouch-AdvanceMedia.xml \
				  opt/Brother-Ptouch-Align.xml \
				  opt/Brother-Ptouch-BytesPerLine.xml \
				  opt/Brother-Ptouch-ConcatPages.xml \
				  opt/Brother-Ptouch-CutMark.xml \
				  opt/Brother-Ptouch-CutMedia.xml \
				  opt/Brother-Ptouch-HalfCut.xml \
				  opt/Brother-Ptouch-SoftwareMirror.xml \
				  opt/Brother-Ptouch-LabelPreamble.xml \
				  opt/Brother-Ptouch-MirrorPrint.xml \
				  opt/Brother-Ptouch-NegativePrint.xml \
				  opt/Brother-Ptouch-PageSize.xml \
				  opt/Brother-Ptouch-PixelTransfer.xml \
				  opt/Brother-Ptouch-PrintDensity.xml \
				  opt/Brother-Ptouch-PrintQuality.xml \
				  opt/Brother-Ptouch-Resolution.xml \
				  opt/Brother-Ptouch-RollFedMedia.xml \
				  opt/Brother-Ptouch-TransferMode.xml

EXTRA_DIST			= Doxyfile $(PACKAGE).spec.in \
				  $(PACKAGE)-foomatic.spec.in

RPMBUILD			= rpmbuild -ba
RPMSOURCESDIR			= $(HOME)/src/SOURCES

dist-rpm: $(RPMSOURCESDIR)/$(distdir).tar.gz \
	$(PACKAGE).spec $(PACKAGE)-foomatic.spec
	$(RPMBUILD) $(PACKAGE).spec
	$(RPMBUILD) $(PACKAGE)-foomatic.spec

$(RPMSOURCESDIR)/$(distdir).tar.gz: $(distdir).tar.gz
	cp $< $@

%.spec: $(srcdir)/%.spec.in $(top_builddir)/config.status
	@case '$?' in \
	  *config.status*) \
	    echo ' $(SHELL) ./config.status'; \
	    $(SHELL) ./config.status;; \
	  *) \
	    echo ' cd $(top_builddir) && $(SHELL) ./config.status $@ $(am__depfiles_maybe)'; \
	    cd $(top_builddir) && $(SHELL) ./config.status $@ $(am__depfiles_maybe);; \
	esac;
