pcsx2_dir = $(top_srcdir)/..
common_dir = $(pcsx2_dir)/common/include
third_dir = $(pcsx2_dir)/3rdparty
cdvd_dir = $(top_srcdir)/CDVD
x86_dir = $(top_srcdir)/x86
rdebug_dir = $(top_srcdir)/RDebug
debugtools_dir = $(top_srcdir)/DebugTools
ipu_dir = $(top_srcdir)/IPU
linux_dir = $(top_srcdir)/Linux

INCLUDES = -I$(top_srcdir) -I$(x86_dir) -I$(common_dir) -I$(third_dir)
noinst_LIBRARIES = libps2_cdvd.a

libps2_cdvd_a_SOURCES = \
CDVD.cpp CDVDiso.cpp  CDVDisodrv.cpp  CdRom.cpp \
CDVD.h CDVD_internal.h CDVDiso.h CDVDisodrv.h CDVDlib.h CdRom.h 

#SUBDIRS =