# This whole makefile should be converted to the single-makefile style

# Test programs:
TSTPGM = \
	TestActuator \
	TestActuatorAsync \
	TestGenerator \
	TestIO \
	TestRandom \
	TestRandomFixedSeed \
	TestStats \
	TestStore \
	TestTimer
TSTOBJ = $(patsubst %,%.o,$(TSTPGM))
TSTSRC = $(patsubst %,%.C,$(TSTPGM))

# Real programs:
PGM = \
	Pharos \
	GenerateMacroExperiments

PGMOBJ = $(patsubst %,%.o,$(PGM))
PGMSRC = $(patsubst %,%.C,$(PGM))

# All objects that can be made.  The order shows the inheritance.  The
# objects for programs are omitted.
OBJ = $(TSTOBJ) $(PGMOBJ) \
	Actuator.o ActuatorAsync .o \
	IO.o IOStat.o PerfStat.o \
	Timer.o \
	Store.o \
	Generator.o GeneratorStackable.o \
          GeneratorAFAP.o GeneratorUniform.o GeneratorPoisson.o GeneratorPeriodic.o GeneratorReplayTrace.o\
          GeneratorConstSize.o  GeneratorRandomSize.o\
	  GeneratorLocation.o GeneratorSeq.o GeneratorSeqRun.o GeneratorRandom.o \
        Stream.o StreamSingleSync.o StreamMultiSync.o \
	Experiment.o \
	Random.o RandomBool.o RandomUniform.o RandomExponential.o \
	BaseRandom.o BaseRandom48.o \
	printe.o
OBJSRC = $(patsubst %.o,%.C,$(OBJ))

HDR = \
	Actuator.H ActuatorAsync.H \
	IO.H IOStat.H \
	Store.H \
	Timer.H \
	BaseRandom.H BaseRandom48.H \
	Generator.H GeneratorStackable.H \
          GeneratorAFAP.H GeneratorUniform.H GeneratorPoisson.H GeneratorPeriodic.H GeneratorReplayTrace.H \
          GeneratorConstSize.H GeneratorRandomSize.H\
	  GeneratorLocation.H GeneratorSeq.H GeneratorSeqRun.H GeneratorRandom.H \
        Stream.H StreamSingleSync.H StreamMultiSync.H\
	Experiment.H \
	Random.H RandomBool.H RandomUniform.H RandomExponential.H \
	Reportable.H Stats.H \
	Types.H \
	PerfStat.H \
	print.H

pgm: $(PGM)
all: $(PGM) $(TSTPGM)

# If you are using the stock glibc version of Posix async IO, then enable
# the following switches:
# AIO_H_DEP = 
# AIO_H_LOC =
# AIO_LIB_DEP =
# AIO_LIB_LOC = -lrt
# Warning: This does not work for the Fedora 3 distributed kernel/glibc

# If you are using the Posix AIO library, enable the following:
AIO_VERSION = 0.8.1
AIO_H_DEP = aio.h
AIO_H_LOC = -I. 
AIO_LIB_DEP = libposix-aio.a
AIO_LIB_LOC =

# All the following rules are just there to build the libposix-aio stuff:
libposix-aio-$(AIO_VERSION)/Makefile: libposix-aio-$(AIO_VERSION).tar.gz
	tar xzf $<
	( cd libposix-aio-$(AIO_VERSION); ./configure )
aio.h: libposix-aio-$(AIO_VERSION)/Makefile
	( cd libposix-aio-$(AIO_VERSION); make )
	cp libposix-aio-$(AIO_VERSION)/include/aio.h ./
libposix-aio.a: libposix-aio-$(AIO_VERSION)/Makefile
	( cd libposix-aio-$(AIO_VERSION); make )
	cp libposix-aio-$(AIO_VERSION)/src/.libs/libposix-aio.a ./

# The following needs to be generalized for other OSes
CXX = g++
LD = g++
# On Fedora, only _LARGEFILE_SOURCE is needed.
# On RH7, all three defines are needed.
# I hate computers
CXXFLAGS = -std=c++0x -Wall -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE=1
LDFLAGS = -pthread

%.o: %.C
	$(CXX) $(CXXFLAGS) -g $(AIO_H_LOC) -c $<

%: %.o
	$(LD) $(LDFLAGS) -g $^ -o $@

# TODO: Mark those as non-targets the way gnu make likes it
pretty:
	rm -f *~ testfile testtrace

clean: pretty
	rm -f $(TSTPGM) $(PGM)
	rm -f $(OBJ)

mrproper: clean
	rm -Rf libposix-aio-$(AIO_VERSION)

testpgm: $(TSTPGM)
testexec = $(foreach test,$(TSTPGM),./$(test);)
test: $(TSTPGM)
	$(testexec)

# TODO: The -I flag works on RH7.0
#       On Fedora, replace it with -j
pharos.tar.bz2: $(TSTSRC) $(PGMSRC) $(OBJSRC) $(HDR) Makefile
	tar -cIf pharos.tar.bz2 $^

# Link dependencies:
TestActuator: \
	Actuator.o IOStat.o IO.o Store.o Timer.o Experiment.o Generator.o printe.o

TestActuatorAsync: \
	TestActuatorAsync.o Actuator.o ActuatorAsync.o \
	IOStat.o IO.o Store.o Timer.o Experiment.o Generator.o \
	printe.o \
	$(AIO_LIB_DEP)
	$(LD) $(LDFLAGS) -g --print-map $^ $(AIO_LIB_LOC) -o $@ 

TestGenerator: \
	IO.o Store.o \
	Generator.o GeneratorConstSize.o GeneratorRandomSize.o eneratorAFAP.o \
	GeneratorRandom.o GeneratorSeqRun.o GeneratorSeq.o RandomUniform.o \
	Random.o BaseRandom.o BaseRandom48.o \
	printe.o

TestIO: \
	IOStat.o IO.o Timer.o printe.o

TestRandom: \
	BaseRandom.o BaseRandom48.o \
	Random.o RandomBool.o RandomUniform.o    

TestRandomFixedSeed: \
	BaseRandom.o BaseRandom48.o

TestStats: \
	TestStats.o Stats.o \
	printe.o

TestStore: \
	Store.o Timer.o \
	printe.o

TestTimer: \
	Timer.o

Pharos: \
	Pharos.o \
	IO.o IOStat.o Store.o Actuator.o ActuatorAsync.o \
	Timer.o \
	Generator.o GeneratorStackable.o \
	GeneratorAFAP.o GeneratorUniform.o GeneratorPoisson.o GeneratorPeriodic.o GeneratorReplayTrace.o\
	GeneratorConstSize.o GeneratorRandomSize.o\
	GeneratorLocation.o GeneratorRandom.o GeneratorSeqRun.o GeneratorSeq.o \
        Stream.o StreamSingleSync.o StreamMultiSync.o \
	Experiment.o \
        RandomUniform.o RandomExponential.o \
	Random.o BaseRandom.o BaseRandom48.o \
	PerfStat.o printe.o \
	$(AIO_LIB_DEP)
	$(LD) $(LDFLAGS) -g $^ $(AIO_LIB_LOC) -o $@

GenerateMacroExperiments: \
	BaseRandom.o BaseRandom48.o RandomUniform.o Random.o \
	printe.o

# General dependencies:

# ActuatorAsync uses the aio.h header file:
ActuatorAsync.o: $(AIO_H_DEP)

# TODO: They should be made or checked automatically
Actuator.o: Actuator.H IOStat.H IO.H Store.H Timer.H Experiment.H Generator.H printe.H
ActuatorAsync.o: Actuator.H ActuatorAsync.H IOStat.H IO.H Store.H Timer.H Experiment.H Generator.H printe.H
BaseRandom.o: BaseRandom.H Types.H
BaseRandom48.o: BaseRandom48.H BaseRandom.H Types.H
Experiment.o: Experiment.H
Generator.o: Generator.H IO.H
GeneratorAFAP.o: GeneratorAFAP.H IO.H Generator.H
GeneratorConstSize.o: GeneratorConstSize.H IO.H GeneratorStackable.H Generator.H
GeneratorRandomSize.o: GeneratorRandomSize.H IO.H GeneratorStackable.H Generator.H
GeneratorExponential.o: GeneratorExponential.H Generator.H
GeneratorLocation.o: IO.H GeneratorLocation.H GeneratorStackable.H Generator.H Random.H
GeneratorRandom.o: GeneratorRandom.H IO.H GeneratorStackable.H Generator.H Store.H RandomUniform.H printe.H
GeneratorSeqRun.o: GeneratorSeqRun.H IO.H GeneratorStackable.H Generator.H Store.H RandomUniform.H printe.H
GeneratorSeq.o: GeneratorSeq.H IO.H GeneratorStackable.H Generator.H Store.H printe.H
GeneratorStackable.o: Generator.H
GeneratorUniform.o: GeneratorUniform.H Generator.H
IOStat.o: IOStat.H IO.H Timer.H printe.H
Pharos.o: IO.H IOStat.H Store.H Actuator.H ActuatorAsync.H Timer.H GeneratorStackable.H Generator.H GeneratorConstSize.H GeneratorRandomSize.H GeneratorAFAP.H GeneratorUniform.H GeneratorPoisson.H GeneratorPeriodic.H GeneratorRandom.H GeneratorSeqRun.H GeneratorSeq.H Random.H RandomUniform.H RandomExponential.H Types.H BaseRandom.H Experiment.H printe.H
PerfStat.o: PerfStat.H
Random.o: Random.H Types.H
RandomBool.o: RandomBool.H BaseRandom.H Types.H
RandomExponential.o: RandomExponential.H Random.H BaseRandom.H Types.H
RandomUniform.o: RandomUniform.H Random.H BaseRandom.H Types.H
Store.o: Store.H IO.H printe.H
Stream.o: Stream.H Actuator.H IOStat.H IO.H Store.H Timer.H Generator.H printe.H
StreamSingleSync.H: StreamSingleSync.H Stream.H Actuator.H IOStat.H IO.H Store.H Timer.H Generator.H Experiment.H printe.H
StreamMultiSync.o: StreamMultiSync.H Stream.H Actuator.H IOStat.H IO.H Store.H Timer.H Generator.H Experiment.H printe.H PerfStat.H
TestActuator.o: Actuator.H IOStat.H IO.H Store.H Timer.H Experiment.H Generator.H printe.H
TestActuatorAsync.o: Actuator.H ActuatorAsync.H IOStat.H IO.H Store.H Timer.H Experiment.H Generator.H printe.H
TestGenerator.o: IO.H Store.H GeneratorStackable.H Generator.H GeneratorConstSize.H GeneratorRandomSize.H GeneratorAFAP.H GeneratorRandom.H GeneratorSeqRun.H GeneratorSeq.H Random.H RandomUniform.H Types.H BaseRandom.H printe.H
TestIO.o: Timer.H IO.H IOStat.H printe.H
TestRandom.o: BaseRandom48.H BaseRandom.H Types.H
TestRandomFixedSeed.o: BaseRandom48.H BaseRandom.H Types.H
TestTimer.o: Timer.H
GenerateMacroExperiments.o: BaseRandom.H BaseRandom48.H Types.H RandomUniform.H Random.H printe.H
