ifndef EXE
	EXE = tarnished
endif

EXE_SUFFIX =
LDFLAGS = -fuse-ld=lld
ifeq ($(OS), Windows_NT)
	EXE_SUFFIX = .exe
endif

.ONESHELL:
$(EXE):
	if not exist build mkdir build
	cd build
	cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build .
	move tarnished.exe ../$(EXE)$(EXE_SUFFIX)
