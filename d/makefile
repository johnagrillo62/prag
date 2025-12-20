

# ====================================================================
# Makefile for Building liblang.a and Linking
# ====================================================================

DMD      = dmd
LIBNAME  = liblang.a

LIBSOURCES = \
	core/generator.d \
	core/types.d \
	core/cpp.d \
	core/csv.d \
	core/go.d \
	core/haskell.d \
	core/java.d \
	core/python.d \
	core/ruby.d \
	core/rust.d \
	core/sql.d \
	core/typescript.d \
	core/xml.d \
	core/yaml.d \
	core/zig.d

DEMO     = demo
DEMO_SOURCES = demo.d models.d

.PHONY: all lib demo bhw clean

all: lib $(DEMO) 

# Build the static library
lib: $(LIBNAME)

$(LIBNAME): $(LIBSOURCES)
	$(DMD) -lib -of$@ $(LIBSOURCES)

# Build demo executable linking liblang.a
$(DEMO): $(DEMO_SOURCES) lib
	$(DMD) $(DEMO_SOURCES) \
		-L-L. \
		-L-l:liblang.a \
		-of$(DEMO)

clean:
	rm -f $(LIBNAME) $(DEMO) $(BHW) *.o *.obj *.exe *.di

