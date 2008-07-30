# Backward compatibility.
include_HEADERS =				\
include/uobject.h

urbiinclude_HEADERS =				\
include/urbi/qt_umain.hh			\
include/urbi/uabstractclient.hh			\
include/urbi/uclient.hh				\
include/urbi/uconversion.hh			\
include/urbi/uexternal.hh			\
include/urbi/umain.hh				\
include/urbi/usyncclient.hh			\
include/urbi/utag.hh

# liburbi is installed in $prefix, and its headers depend on libport.
# So we must provide libport in $prefix, although they are also
# installed as part of the SDK.  Using symlinks is not portable.
#include_libport_dir = $(includedir)/libport
#nodist_include_libport__HEADERS = $(libport_HEADERS) $(nodist_libport_HEADERS)
#include_libport_sys_dir = $(include_libport_dir)/sys
#nodist_include_libport_sys__HEADERS = $(libportsys_HEADERS)

include_libport = include/libport
include include/libport/libport.mk
