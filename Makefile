.PHONY: all build install test clean distclean

BUILD_DIR := build/debug
PRESET := debug

all: install

build:
	cmake --preset $(PRESET)
	cmake --build --preset $(PRESET)

install: build
	cmake --install "$(BUILD_DIR)" --prefix "$(CURDIR)"

test: build
	ctest --preset $(PRESET)

clean:
	cmake -E rm -rf "$(BUILD_DIR)"

distclean:
	cmake -E rm -rf .cache build bin
