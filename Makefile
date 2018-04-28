# ------------------------------------
# Makefile for a Linux or OSX platform
# Guy Turcotte April 2018
# ------------------------------------

# ----- Compiler and Linker -----

CC          := g++

# ----- The Target Binary Program -----

TARGET      := mezzo

# ----- The Directories, Source, Includes, Objects, Binary and Resources -----

SRCDIR       := src
INCDIR       := src/include
BUILDDIR     := obj
TARGETDIR    := bin
SRCEXT       := cpp
DEPEXT       := d
OBJEXT       := o
BOOST_INCDIR := /usr/local/include/boost
BOOST_LIBDIR := /usr/local/lib



# ----- Flags, Libraries and Includes -----

CFLAGS      := -std=gnu++14 -pthread -c -W -Wall -Wextra -pedantic -march=native -msse3 \
               -Wno-char-subscripts -Wno-unused-function -O3 -g -fno-inline

# CFLAGS      := -std=gnu++14 -pthread -c -W -Wall -Wextra -pedantic -march=native -msse3 \
#               -Wno-char-subscripts -Wno-unused-function -D__LINUX_ALSA__ -O3 -g -fno-inline


BOOST_LIBS  := -lboost_iostreams -lboost_program_options
LIB         := -L$(BOOST_LIBDIR) $(BOOST_LIBS) -lportaudio -pthread  -lrtmidi
INC         := -I$(INCDIR) -I$(BOOST_INCDIR)
INCDEP      := -I$(INCDIR) -I$(BOOST_INCDIR)

# ------------------------------------------------------------------------------
#    DO NOT EDIT BELOW THIS LINE
# ------------------------------------------------------------------------------

SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS     := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

# ----- Default Make -----

all: resources $(TARGETDIR)/$(TARGET)

# ----- Remake -----

remake: cleaner all

run:
	LD_LIBRARY_PATH=$(BOOST_LIBS) && $(TARGETDIR)/$(TARGET) /data/sf2/FluidR3_GM.sf2

# ----- Copy Resources from Resources Directory to Target Directory -----

resources: directories

# ----- Make the Directories -----

directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

# ----- Clean only Objects -----

clean:
	@$(RM) -f $(BUILDDIR)/*

# ----- Full Clean, Objects and Binaries -----

cleaner: clean
	@$(RM) -f $(TARGETDIR)/*

# ----- Pull in dependency info for *existing* .o files -----

-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

# ----- Link -----

$(TARGETDIR)/$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGETDIR)/$(TARGET) $^ $(LIB)

# ----- Compile -----

$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

# ----- Non-File Targets -----

.PHONY: all run remake clean cleaner resources
