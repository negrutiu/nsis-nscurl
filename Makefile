
.PHONY: all clean

clean:
	$(MAKE) -C src/nscurl clean

all:
	$(MAKE) -C src/nscurl all
