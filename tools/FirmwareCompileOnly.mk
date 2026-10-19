# Include after SDK/Makefile; deliberately omit the download/post-build target.
.PHONY: compile-only
compile-only: pre_build
	$(MAKE) -f build/Makefile.mk pre_build
	$(MAKE) -f build/Makefile.mk cpu/br33/tools/sdk.elf
