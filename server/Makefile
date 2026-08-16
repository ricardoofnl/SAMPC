# linux build for the SA:MP server, 32 bit only
#
# needs the 32 bit toolchain on a 64 bit host:
#   fedora: dnf install glibc-devel.i686 libstdc++-devel.i686
#   debian: apt install gcc-multilib g++-multilib
#
# no -fpack-struct, deliberately. the old Makefile had it and it broke two things
# at once: AIM_SYNC_DATA became 31 bytes where MSVC makes 36 and those structs go
# over the wire raw via sizeof(), and std::istringstream got a packed layout that
# libstdc++ does not share, smashing the stack in CPlugins::LoadPlugins

CC  = gcc
CXX = g++
RM  = rm -rf

TARGET = mpsvr
OBJDIR = obj

DEFS   = -DLINUX -DSAMPSRV -DHAVE_STDINT_H -DAMX_NODYNALOAD
INC    = -I../sdk -I../sdk/amx
# the stack protector stays on by default, this is a network facing server and
# the codebase has a history of unchecked lengths coming off the wire
COMMON = -m32 -w -fstack-protector-strong $(DEFS) $(INC)

OPT   = -O2
DEPS  = -MMD -MP
BASE  = $(COMMON) $(OPT) $(DEPS)

# server sources stay strict, -fpermissive here would hide real type errors and
# leave MSVC to catch them first
CXXFLAGS = $(BASE)

# vendored raknet and pawn amx need it, modern gcc rejects their old habits
VENDOR_CXXFLAGS = $(BASE) -fpermissive
CFLAGS          = $(BASE) -fpermissive

LDFLAGS  = -m32
LIBS     = -ldl -lpthread -lm

SRV_SRCS = $(wildcard *.cpp)

# only the RakNet units the server actually needs, kept in sync with
# server.vcxproj
RAK_SRCS = \
  ../sdk/raknet/_findfirst.cpp \
  ../sdk/raknet/BitStream.cpp \
  ../sdk/raknet/CheckSum.cpp \
  ../sdk/raknet/DataBlockEncryptor.cpp \
  ../sdk/raknet/DataCompressor.cpp \
  ../sdk/raknet/DirectoryDeltaTransfer.cpp \
  ../sdk/raknet/DS_HuffmanEncodingTree.cpp \
  ../sdk/raknet/DS_Table.cpp \
  ../sdk/raknet/FileList.cpp \
  ../sdk/raknet/FileListTransfer.cpp \
  ../sdk/raknet/FileOperations.cpp \
  ../sdk/raknet/GetTime.cpp \
  ../sdk/raknet/InternalPacketPool.cpp \
  ../sdk/raknet/LinuxStrings.cpp \
  ../sdk/raknet/NatPunchthrough.cpp \
  ../sdk/raknet/NetworkIDGenerator.cpp \
  ../sdk/raknet/NetworkTypes.cpp \
  ../sdk/raknet/PluginInterface.cpp \
  ../sdk/raknet/RakNetStatistics.cpp \
  ../sdk/raknet/RakPeer.cpp \
  ../sdk/raknet/RakServer.cpp \
  ../sdk/raknet/RakSleep.cpp \
  ../sdk/raknet/Rand.cpp \
  ../sdk/raknet/ReliabilityLayer.cpp \
  ../sdk/raknet/rijndael.cpp \
  ../sdk/raknet/RPCMap.cpp \
  ../sdk/raknet/SAMPCipher.cpp \
  ../sdk/raknet/SHA1.cpp \
  ../sdk/raknet/SimpleMutex.cpp \
  ../sdk/raknet/SocketLayer.cpp \
  ../sdk/raknet/StringCompressor.cpp \
  ../sdk/raknet/StringTable.cpp \
  ../sdk/raknet/SystemAddressList.cpp \
  ../sdk/raknet/TableSerializer.cpp \
  ../sdk/raknet/TCPInterface.cpp \
  ../sdk/raknet/TEABlockEncryptor.cpp

SHA2_SRCS = ../sdk/sha2/sha256.cpp

AMX_SRCS = \
  ../sdk/amx/amx.c \
  ../sdk/amx/amxcore.c \
  ../sdk/amx/amxfile.c \
  ../sdk/amx/amxfloat.c \
  ../sdk/amx/amxstring.c \
  ../sdk/amx/amxtime.c

SQLITE_SRCS = ../sdk/sqlite/sqlite3.c

VENDOR_CXX_SRCS = $(RAK_SRCS) $(SHA2_SRCS)
CXX_SRCS        = $(SRV_SRCS) $(VENDOR_CXX_SRCS)
C_SRCS          = $(AMX_SRCS) $(SQLITE_SRCS)

# flatten the paths so objects from different directories cannot collide
OBJS = $(addprefix $(OBJDIR)/,$(subst /,_,$(CXX_SRCS:.cpp=.o) $(C_SRCS:.c=.o)))

all: $(TARGET)

# recursive call so the overrides are in effect while OBJS and the rules below
# get expanded, a target specific variable would be too late for that
debug:
	$(MAKE) OPT="-O0 -g3" OBJDIR=obj-debug TARGET=mpsvrd

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# one rule per source, the object name is the source path with the separators
# replaced so files of the same name in different directories cannot collide
define OBJ_RULE_CXX
$(OBJDIR)/$(subst /,_,$(basename $(1))).o: $(1)
	@mkdir -p $(OBJDIR)
	$$(CXX) $$(CXXFLAGS) -c $$< -o $$@
endef

define OBJ_RULE_VENDOR_CXX
$(OBJDIR)/$(subst /,_,$(basename $(1))).o: $(1)
	@mkdir -p $(OBJDIR)
	$$(CXX) $$(VENDOR_CXXFLAGS) -c $$< -o $$@
endef

define OBJ_RULE_C
$(OBJDIR)/$(subst /,_,$(basename $(1))).o: $(1)
	@mkdir -p $(OBJDIR)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
endef

$(foreach s,$(SRV_SRCS),$(eval $(call OBJ_RULE_CXX,$(s))))
$(foreach s,$(VENDOR_CXX_SRCS),$(eval $(call OBJ_RULE_VENDOR_CXX,$(s))))
$(foreach s,$(C_SRCS),$(eval $(call OBJ_RULE_C,$(s))))

-include $(OBJS:.o=.d)

.PHONY: all debug clean

clean:
	$(RM) obj obj-debug $(TARGET) mpsvrd