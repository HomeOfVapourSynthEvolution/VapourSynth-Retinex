#----------------------------------------------------------------------------
#  Makefile for VapourSynth-Retinex, modified based on d2vsource/GNUmakefile
#----------------------------------------------------------------------------

include config.mak

vpath %.cpp $(SRCDIR)
vpath %.h $(SRCDIR)

SRCS = source/VSPlugin.cpp source/Gaussian.cpp source/MSR.cpp source/MSRCP.cpp source/MSRCR.cpp

OBJS = $(SRCS:%.cpp=%.o)

.PHONY: all install clean distclean dep

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $^ $(LIBS)
	-@ $(if $(STRIP), $(STRIP) -x $@)

%.o: %.cpp .depend
	$(CXX) $(CXXFLAGS) -c $< -o $@

install: all
	install -d $(libdir)
	install -m 755 $(LIBNAME) $(libdir)

clean:
	$(RM) *.dll *.so *.dylib $(OBJS) .depend

distclean: clean
	$(RM) config.*

dep: .depend

ifneq ($(wildcard .depend),)
include .depend
endif

.depend: config.mak
	@$(RM) .depend
	@$(foreach SRC, $(SRCS:%=$(SRCDIR)/%), $(CXX) $(SRC) $(CXXFLAGS) -MT $(SRC:$(SRCDIR)/%.cpp=%.o) -MM >> .depend;)

config.mak:
	./configure
