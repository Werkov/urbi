## Copyright (C) 2005-2012, Gostai S.A.S.
##
## This software is provided "as is" without warranty of any kind,
## either expressed or implied, including but not limited to the
## implied warranties of fitness for a particular purpose.
##
## See the LICENSE file for more information.

## Use the versioning information of the Kernel.
include $(top_srcdir)/build-aux/make/init.mk
include $(top_srcdir)/build-aux/make/fix-libtool-la.mk
include $(top_srcdir)/build-aux/make/libport.mk

SUBDIRS = . tests
if BINDING_JAVA
SUBDIRS += swig/java
endif

# libjpeg.
JPEG_CPPFLAGS = -I$(sdk_remote_srcdir)/jpeg -I$(sdk_remote_builddir)/jpeg
JPEG_LIBS = $(sdk_remote_builddir)/jpeg/libjpeg4urbi$(LIBSFX).la

SDK_REMOTE_CPPFLAGS = 				\
  $(LIBPORT_CPPFLAGS)				\
  $(BOOST_CPPFLAGS)				\
  -I$(sdk_remote_srcdir)/include		\
  -I$(sdk_remote_builddir)/include

AM_CPPFLAGS +=					\
  $(SDK_REMOTE_CPPFLAGS)			\
  $(JPEG_CPPFLAGS)				\
  -I$(srcdir)

AM_CXXFLAGS += $(PTHREAD_CFLAGS) $(SOCKETS_CFLAGS) $(WARNING_CXXFLAGS)

LIBADD =					\
  $(PTHREAD_LIBS)				\
  $(SOCKETS_LIBS)				\
  $(LIBPORT_LIBS)

# To be linked in all the executables (urbi-launch, examples/ etc.).
AM_LDADD =					\
  liburbi/liburbi$(LIBSFX).la			\
  $(JPEG_LIBS)					\
  $(PTHREAD_LIBS)				\
  $(LIBPORT_LIBS)

# The order matters: build dependencies first.
include libuvalue/local.mk
include libuco/local.mk

.PHONY: installcheck-umake
if SDK_REMOTE
include bin/local.mk
include liburbi/local.mk

if !STATIC_BUILD
include uobjects/local.mk
endif !STATIC_BUILD

include libuobject/local.mk
endif SDK_REMOTE

if EXAMPLES
include examples/local.mk
include utils/local.mk
endif EXAMPLES


# Replace the version stubs in these libraries.  They get it from
# revision.cc in libuvalue, which is a convenience library included in
# these two.
.PHONY: stub.stamp
VERSIONIFY_STUBS = 					\
  libuco/libuco.la					\
  libuobject/libuobject$(LIBSFX).la			\
  liburbi/liburbi$(LIBSFX).la				\
  libuvalue/libuvalue.la				\
  libuvalue/libuvalue_libuvalue_la-package-info.lo	\
  $(UOBS_LIBS)

stub.stamp: $(VERSIONIFY_STUBS) $(VERSIONIFY_CACHE) $(VERSIONIFY)
	$(AT_V_PATCH) $(VERSIONIFY_RUN)				\
	  $(addprefix --resolve=, $(VERSIONIFY_STUBS))
ALLS += stub.stamp





## -------- ##
## Stamps.  ##
## -------- ##

# All the libraries, executables, and everything.
STAMPS += libraries.stamp executables.stamp all.stamp
libraries.stamp: $(LTLIBRARIES)
executables.stamp: $(PROGRAMS)
all.stamp: $(LTLIBRARIES) $(PROGRAMS)
