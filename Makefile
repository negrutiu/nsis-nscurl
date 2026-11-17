.PHONY: % 

%:
	$(MAKE) -C src/nscurl $(MAKECMDGOALS)
