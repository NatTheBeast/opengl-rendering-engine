BUILD_DIR = build
JOBS = $(shell sysctl -n hw.logicalcpu)

.PHONY: all build configure run clean

all: build

configure:
	cmake -S src -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

run:
	cd src && ../$(BUILD_DIR)/TP

clean:
	rm -rf $(BUILD_DIR)
