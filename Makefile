export ROOT_DIR := $(CURDIR)
export BUILD_CONFIG ?= debug
export BUILD_DIR := build

all check test:
	$(MAKE) run -C tests

clean:
	$(RM) -r $(BUILD_DIR)/*

.PHONY: runner runner-reference shared trainer
runner runner-reference shared trainer:
	$(MAKE) -C $@
