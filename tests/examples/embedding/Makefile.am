# Copyright (C) 2018 RidgeRun, LLC (http://www.ridgerun.com)
# All Rights Reserved.
#
# The contents of this software are proprietary and confidential to RidgeRun,
# LLC.  No part of this program may be photocopied, reproduced or translated
# into another programming language without prior written consent of
# RidgeRun, LLC.  The user is free to modify the source code after obtaining
# a software license from RidgeRun.  All source code changes must be provided
# back to RidgeRun without any encumbrance.

if BUILD_EXAMPLES

noinst_PROGRAMS =                  \
        embedding

# sources used to compile this plug-in
embedding_SOURCES = gstembedding.c customlogic.cc customlogic.h

AM_CFLAGS =                         \
        -I\$(top_srcdir)/gst-libs   \
        $(GST_CFLAGS)		    \
        $(GST_BASE_CFLAGS)          \
        $(GST_PLUGINS_BASE_CFLAGS)

AM_LDFLAGS =                        \
        $(GST_LIBS)		    \
        $(GST_BASE_LIBS)	    \
        $(GST_PLUGINS_BASE_LIBS)

endif # BUILD_EXAMPLES
