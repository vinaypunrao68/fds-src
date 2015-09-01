topdir := ../..
user_target := lib
user_rtime_env := user

user_cpp := BlobMetadataKey.cpp \
            BlobObjectKey.cpp \
            JournalTimestampKey.cpp \
            ObjectExpungeKey.cpp \
            ObjectRankKey.cpp \
            VolumeMetadataKey.cpp \
            CatalogKeyComparator.cpp \
            CatalogKey.cpp

user_incl_dir := ../../include/catalogKeys .. $(topdir)

user_ar_libs := fds-lib
fds-lib := $(user_cpp)

include $(topdir)/Makefile.incl
