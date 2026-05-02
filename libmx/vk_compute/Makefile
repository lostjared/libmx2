# Makefile for compiling GLSL shaders to SPIR-V for vk_fractal
GLSLC := glslc

# Shader sources
VERT_SHADERS := $(wildcard *.vert)
FRAG_SHADERS := $(wildcard *.frag)

# SPIR-V outputs
VERT_SPVS := $(VERT_SHADERS:.vert=_vert.spv)
FRAG_SPVS := $(FRAG_SHADERS:.frag=_frag.spv)

# Legacy naming (some shaders use .spv without suffix)
LEGACY_SPVS := vert.spv frag.spv

ALL_SPVS := $(VERT_SPVS) $(FRAG_SPVS) $(LEGACY_SPVS)

.PHONY: all clean help

all: $(ALL_SPVS)

# Compile vertex shaders
%_vert.spv: %.vert
	$(GLSLC) -o $@ $<
	@echo "Compiled $< -> $@"

# Compile fragment shaders
%_frag.spv: %.frag
	$(GLSLC) -o $@ $<
	@echo "Compiled $< -> $@"

# Legacy targets
vert.spv: vertex.vert
	$(GLSLC) -o $@ $<
	@echo "Compiled $< -> $@"

frag.spv: fragment.frag
	$(GLSLC) -o $@ $<
	@echo "Compiled $< -> $@"

clean:
	@rm -f $(ALL_SPVS)
	@echo "Cleaned compiled shaders"

help:
	@echo "Available targets:"
	@echo "  make all     - Compile all shaders"
	@echo "  make clean   - Remove all compiled shader files"
	@echo "  make help    - Display this help message"

.PHONY: list
list:
	@echo "Vertex shaders: $(VERT_SHADERS)"
	@echo "Fragment shaders: $(FRAG_SHADERS)"
