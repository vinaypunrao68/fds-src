topdir            := ../../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) ../../include
user_cpp_flags    :=
user_cpp          := \
	DmVolumeCatalog.cpp \
	DmPersistVolCat.cpp \
	DmPersistVolDB.cpp \
	CatalogKey.cpp \
	CatalogKeyComparator.cpp \
	BlobObjectKey.cpp \
	VolumeMetadataKey.cpp \
	JournalTimestampKey.cpp
#	DmOIDArrayMmap.cpp \
#	DmPersistVolFile.cpp

user_ar_libs      := fds-dm-lib
fds-dm-lib        := $(user_cpp)

include $(topdir)/Makefile.incl
