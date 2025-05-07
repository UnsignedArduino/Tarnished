# sp yoink

ifndef EXE
    EXE = tarnished
endif

EXE_SUFFIX =
LDFLAGS = -fuse-ld=lld
ifeq ($(OS), Windows_NT)
	EXE_SUFFIX = .exe
endif

SOURCES := src/*.cpp
CXX := clang++
CXXFLAGS := -std=c++20 -O3 -flto -DNDEBUG -march=native

$(EXE)$(EXE_SUFFIX): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SOURCES) -o $(EXE)$(EXE_SUFFIX)