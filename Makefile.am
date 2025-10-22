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

LIBM				= @LIBM@
LIBCUPS				= @LIBCUPS@
LIBCUPSIMAGE			= @LIBCUPSIMAGE@
LIBPNG				= @LIBPNG@

execfilterdir			= $(libdir)/cups/filter
execfilter_PROGRAMS		= rastertoptch
rastertoptch_CPPFLAGS		= -D_GNU_SOURCE
rastertoptch_LDADD		= $(LIBM) $(LIBCUPS) $(LIBCUPSIMAGE)

noinst_PROGRAMS			= ptexplain
ptexplain_LDADD			= $(LIBPNG)

DRIVERS				= driver/ptouch-pt.xml \
				  driver/ptouch-ql.xml
PRINTERS			= printer/Brother-QL-500.xml \
				  printer/Brother-QL-550.xml \
				  printer/Brother-QL-570.xml \
				  printer/Brother-QL-600.xml \
				  printer/Brother-QL-650TD.xml \
				  printer/Brother-QL-700.xml \
				  printer/Brother-QL-800.xml \
				  printer/Brother-QL-810W.xml \
				  printer/Brother-QL-820NWB.xml \
				  printer/Brother-PT-PC.xml \
				  printer/Brother-PT-18R.xml \
				  printer/Brother-PT-550A.xml \
				  printer/Brother-PT-E550W.xml \
				  printer/Brother-PT-P300BT.xml \
				  printer/Brother-PT-P700.xml \
				  printer/Brother-PT-P710BT.xml \
				  printer/Brother-PT-P900W.xml \
				  printer/Brother-PT-P950NW.xml \
				  printer/Brother-PT-1500PC.xml \
				  printer/Brother-PT-1950VP.xml \
				  printer/Brother-PT-1950.xml \
				  printer/Brother-PT-1960.xml \
				  printer/Brother-PT-2300.xml \
				  printer/Brother-PT-2420PC.xml \
				  printer/Brother-PT-2430PC.xml \
				  printer/Brother-PT-2450DX.xml \
				  printer/Brother-PT-2500PC.xml \
				  printer/Brother-PT-2600.xml \
				  printer/Brother-PT-2610.xml \
				  printer/Brother-PT-2700.xml \
				  printer/Brother-PT-3600.xml \
				  printer/Brother-PT-9200DX.xml \
				  printer/Brother-PT-9200PC.xml \
				  printer/Brother-PT-9400.xml \
				  printer/Brother-PT-9500PC.xml \
				  printer/Brother-PT-9600.xml \
				  printer/Brother-PT-9700PC.xml
OPTIONS				= opt/Brother-PTQL-Align.xml \
				  opt/Brother-PTQL-AutoCut.xml \
				  opt/Brother-PTQL-ChainPrinting.xml \
				  opt/Brother-PTQL-BytesPerLine.xml \
				  opt/Brother-PTQL-ConcatPages.xml \
				  opt/Brother-PTQL-CutLabel.xml \
				  opt/Brother-PTQL-LabelPreamble.xml \
				  opt/Brother-PTQL-LastPageFlag.xml \
				  opt/Brother-PTQL-MinMargin.xml \
				  opt/Brother-PTQL-MirrorPrint.xml \
				  opt/Brother-PTQL-NegativePrint.xml \
				  opt/Brother-PTQL-PixelTransfer.xml \
				  opt/Brother-PTQL-Resolution.xml \
				  opt/Brother-PTQL-Series.xml \
				  opt/Brother-PTQL-SoftwareMirror.xml \
				  opt/Brother-PTQL-TransferMode.xml \
				  opt/Brother-PT-HalfCut.xml \
				  opt/Brother-PT-LabelRecovery.xml \
				  opt/Brother-PT-LegacyHires.xml \
				  opt/Brother-PT-LegacyTransferMode.xml \
				  opt/Brother-PT-Margin.xml \
				  opt/Brother-PT-PageSize.xml \
				  opt/Brother-QL-CutMark.xml \
				  opt/Brother-QL-Margin.xml \
				  opt/Brother-QL-MediaType.xml \
				  opt/Brother-QL-PageSize.xml \
				  opt/Brother-QL-PrintDensity.xml \
				  opt/Brother-QL-PrintQuality.xml \
				  opt/Brother-QL-StatusNotification.xml

foomaticdbdir			= $(datadir)/foomatic/db/source
driversdir			= $(foomaticdbdir)/driver
drivers_DATA			= $(addprefix generated/,$(DRIVERS))
printersdir			= $(foomaticdbdir)/printer
printers_DATA			= $(addprefix generated/,$(PRINTERS))
optionsdir			= $(foomaticdbdir)/opt
options_DATA			= $(addprefix generated/,$(OPTIONS))

EXTRA_DIST			= Doxyfile $(PACKAGE).spec.in \
				  $(PACKAGE)-foomatic.spec.in \
				  $(DRIVERS) $(PRINTERS) $(OPTIONS) \
				  foomaticalize

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

export PACKAGE_STRING
$(addprefix generated/,$(DRIVERS)): generated/stamp-h1
$(addprefix generated/,$(PRINTERS)): generated/stamp-h1
$(addprefix generated/,$(OPTIONS)): generated/stamp-h1
generated/stamp-h1: $(DRIVERS) $(PRINTERS) $(OPTIONS) foomaticalize
	$(srcdir)/foomaticalize --srcdir=$(srcdir) --out=generated $(DRIVERS) $(PRINTERS) $(OPTIONS)
	@touch $@

clean-local:
	-rm -rf generated

XMLSCHEMA = /usr/share/foomatic/xmlschema
xmlschema-driver: $(DRIVERS)
	@xmllint --noout --schema $(XMLSCHEMA)/driver.xsd $+
xmlschema-printer: $(PRINTERS)
	@for xml in $+; do \
	    echo -n "$$xml "; \
	    xmlstarlet ed \
		-d '/printer/unverified' \
		-d '/printer/select' \
		"$$xml" \
	    | xmllint --noout --schema $(XMLSCHEMA)/printer.xsd -; \
	done
xmlschema-option: $(OPTIONS)
	@xmllint --noout --schema $(XMLSCHEMA)/option.xsd $+

xmlschema: xmlschema-driver xmlschema-printer xmlschema-option
xmllint:
	@xmllint --noout $(DRIVERS) $(PRINTERS) $(OPTIONS)
