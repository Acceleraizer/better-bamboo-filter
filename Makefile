EXEC=betterbamboo_test.out
SRCDIR=src
DEPDIR=$(SRCDIR)/include
BUILDDIR=build
OBJDIR=$(BUILDDIR)/obj

_OBJ=bamboo.o test.o SpookyV2.o segment.o
OBJ=$(patsubst %,$(OBJDIR)/%,$(_OBJ))
LIBS=-lstdc++

_DEP=bamboo.hpp SpookyV2.h
DEP=$(patsubst %,$(DEPDIR)/%,$(_DEP))


CC=gcc
CFLAGS= -I$(DEPDIR) -Wall -Wno-format -O2


all: dirs $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $^ -o $(BUILDDIR)/$@ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEP)
	$(CC) $(CFLAGS) -c $< -o $@

dirs:
	mkdir -p $(BUILDDIR) $(OBJDIR)



.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o $(BUILDDIR)/$(EXEC)
	rm -df $(OBJDIR) $(BUILDDIR)
