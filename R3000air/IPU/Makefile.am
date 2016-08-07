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
noinst_LIBRARIES = libIPU.a

libIPU_a_SOURCES = IPU.cpp yuv2rgb.cpp coroutine.cpp \
IPU.h yuv2rgb.h coroutine.h \
acoroutine.S

SUBDIRS = mpeg2lib

#libIPU_a_LIBADD = mpeg2lib/libmpeg2IPU.a