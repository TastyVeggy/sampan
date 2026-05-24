.PHONY: test all run test run_test clean build setup distclean
BUILD_DIR := build/debug
PRESET := debug
PROJECT := sampan
MAIN_BIN := $(BUILD_DIR)/src/$(PROJECT)


all: build run

build: 
	cmake --preset $(PRESET)
	cmake --build --preset $(PRESET)

run:
	./$(MAIN_BIN)

test: build run_test

run_test:
	ctest --preset $(PRESET)

clean:
	rm -rf "$(BUILD_DIR)"

distclean:
	rm -rf .cache
	rm -rf build

