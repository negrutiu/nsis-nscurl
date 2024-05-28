
ifeq ($(OS),Windows_NT)
	PLATFORM = windows
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		PLATFORM = linux
	endif
	ifeq ($(UNAME_S),Darwin)
		PLATFORM = macos
	endif
endif

.PHONY: all clean

clean:
	@echo "Cleaning for platform: $(PLATFORM)"
	@$(MAKE) -C src/nscurl -f Makefile.$(PLATFORM) clean

all:
	@echo "Building for platform: $(PLATFORM)"
	@$(MAKE) -C src/nscurl -f Makefile.$(PLATFORM) all
