SRC = src
OBJ = obj
OBJ_BASE := $(OBJ)
LIBCTRMML = lib/libctrmml

CFLAGS = -Wall
LDFLAGS =

ifneq ($(RELEASE),1)
CFLAGS += -g
LIBCTRMML := $(LIBCTRMML)_debug.a
else
OBJ := $(OBJ)/release
CFLAGS += -O2
LDFLAGS += -s
LIBCTRMML := $(LIBCTRMML).a
endif

LDFLAGS_TEST = -lcppunit
ifeq ($(OS),Windows_NT)
	LDFLAGS += -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic
endif

CORE_OBJS = \
	$(OBJ)/track.o \
	$(OBJ)/song.o \
	$(OBJ)/input.o \
	$(OBJ)/mml_input.o \
	$(OBJ)/player.o \
	$(OBJ)/stringf.o \
	$(OBJ)/vgm.o \
	$(OBJ)/driver.o \
	$(OBJ)/wave.o \
	$(OBJ)/riff.o \
	$(OBJ)/conf.o \
	$(OBJ)/platform/md.o \
	$(OBJ)/platform/mdsdrv.o

MMLC_OBJS = \
	$(CORE_OBJS) \
	$(OBJ)/mmlc.o

MDSLINK_OBJS = \
	$(CORE_OBJS) \
	$(OBJ)/platform/mdslink.o

UNITTEST_OBJS = \
	$(CORE_OBJS) \
	$(OBJ)/unittest/test_track.o \
	$(OBJ)/unittest/test_song.o \
	$(OBJ)/unittest/test_input.o \
	$(OBJ)/unittest/test_mml_input.o \
	$(OBJ)/unittest/test_player.o \
	$(OBJ)/unittest/test_vgm.o \
	$(OBJ)/unittest/test_riff.o \
	$(OBJ)/unittest/test_conf.o \
	$(OBJ)/unittest/test_mdsdrv.o \
	$(OBJ)/unittest/main.o

$(OBJ)/%.o: $(SRC)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -MMD -c $< -o $@

all: mmlc mdslink test

lib: $(LIBCTRMML)

$(LIBCTRMML): $(CORE_OBJS)
	@mkdir -p $(@D)
	$(AR) -q $@ $(CORE_OBJS)

mmlc: $(MMLC_OBJS)
	$(CXX) $(MMLC_OBJS) $(LDFLAGS) -o $@

mdslink: $(MDSLINK_OBJS)
	$(CXX) $(MDSLINK_OBJS) $(LDFLAGS) -o $@

unittest: $(UNITTEST_OBJS)
	$(CXX) $(UNITTEST_OBJS) $(LDFLAGS) $(LDFLAGS_TEST) -o $@

clean:
	rm -rf $(OBJ_BASE)

doc:
	doxygen Doxyfile

sample_mml: mmlc sample/idk.vgm sample/passport.vgm sample/sand_light.vgm
	./mmlc sample/idk.mml
	./mmlc sample/passport.mml
	./mmlc sample/sand_light.mml

cleandoc:
	rm -rf doxygen

test: unittest
	./unittest

check: test

.PHONY: all lib test check clean doc cleandoc sample_mml

-include $(OBJ)/*.d $(OBJ)/unittest/*.d $(OBJ)/platform/*.d
