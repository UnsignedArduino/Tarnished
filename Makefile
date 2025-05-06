ifndef EXE
	EXE = tarnished
endif

EXE_SUFFIX =
ifeq ($(OS), Windows_NT)
	EXE_SUFFIX = .exe
endif

.ONESHELL:
$(EXE):
	-mkdir build
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=Release
	cmake --build .
	cmake -E rename tarnished$(EXE_SUFFIX) ../$(EXE)$(EXE_SUFFIX)
