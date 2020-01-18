#!/bin/sh
rm -rf autom4te.cache configure config.*
autoreconf --install || exit 1
