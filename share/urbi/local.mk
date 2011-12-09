## Copyright (C) 2008-2011, Gostai S.A.S.
##
## This software is provided "as is" without warranty of any kind,
## either expressed or implied, including but not limited to the
## implied warranties of fitness for a particular purpose.
##
## See the LICENSE file for more information.

urbidir = $(brandsharedir)/urbi
dist_urbi_DATA := $(call ls_files,share/urbi/*.u)

nodist_urbi_DATA =				\
  share/urbi/package-info.u			\
  share/urbi/platform$(LIBSFX).u
BUILT_SOURCES += $(nodist_urbi_DATA)


## -------------- ##
## PackageInfos.  ##
## -------------- ##

urbi_package_infodir = $(urbidir)/package-info
nodist_urbi_package_info_DATA =			\
  share/urbi/package-info/urbi.u		\
  share/urbi/package-info/libport.u

share/urbi/package-info/urbi.u: $(top_srcdir)/.version $(VERSIONIFY)
	$(AM_V_GEN)$(VERSIONIFY_RUN) --urbiscript=$@ --prefix='Urbi'
	$(AM_V_at)touch $@
share/urbi/package-info/libport.u: $(top_srcdir)/sdk-remote/libport/.version $(VERSIONIFY)
	$(AM_V_GEN)$(VERSIONIFY_RUN) --urbiscript=$@ --prefix='Libport'
	$(AM_V_at)touch $@

urbi.stamp: $(dist_urbi_DATA) $(nodist_urbi_DATA)
