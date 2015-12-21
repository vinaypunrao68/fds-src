topdir           := ../../
user_target      := lib
user_rtime_env   := user
user_incl_dir    := $(topdir)/include/hash

user_cpp         :=      \
    AvalancheTest.cpp    \
    Bitslice.cpp         \
    Bitvec.cpp           \
    City.cpp             \
    CityTest.cpp         \
    crc.cpp              \
    DifferentialTest.cpp \
    Hashes.cpp           \
    KeysetTest.cpp       \
    lookup3.cpp          \
    md5.cpp              \
    MurmurHash1.cpp      \
    MurmurHash2.cpp      \
    MurmurHash3.cpp      \
    Platform.cpp         \
    Random.cpp           \
    sha1.cpp             \
    SpeedTest.cpp        \
    Spooky.cpp           \
    SpookyTest.cpp       \
    Stats.cpp            \
    SuperFastHash.cpp    \
    base64.cpp           \
    Types.cpp

user_cc          := PMurHash.c 
user_no_style    := $(user_cpp) $(user_cc) $(wildcard *.h)
user_ar_libs     := fds-hash
user_so_libs     := fds-hash
fds-hash         := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
