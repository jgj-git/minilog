CC 			= 	g++ -g
BINDIR 		=	./bin
OBJDIR		=	./obj
LIBDIR 		=	./lib
SRCDIR 		=	./src
BASE_DIR 	=	./examples
TB_NAME		?=	tb_top
DUT_NAME	?=	dut
WORKDIRS	= $(OBJDIR) $(LIBDIR) $(BINDIR)

$(LIBDIR)/simulator.o: $(SRCDIR)/simulator.cc $(SRCDIR)/*.hh $(WORKDIRS)
	$(CC) -o $@ -c $< -I$(SRCDIR)

run: $(BINDIR)/sim_$(TB_NAME)
	$<

debug: $(BINDIR)/sim_$(TB_NAME)
	gdb $<

compile: $(BINDIR)/sim_$(TB_NAME)

rtlonly: $(OBJDIR)/$(DUT_NAME)

$(WORKDIRS):
	mkdir -p $@

# TODO: add rtl dependency
# TODO: call HSB generator here so that tb_top can just be a class (???)
$(OBJDIR)/$(TB_NAME).o: $(BASE_DIR)/tb/$(TB_NAME).cc $(SRCDIR)/simulator.hh $(WORKDIRS)
	$(CC) -o $@ -c $< -I$(SRCDIR)

$(BINDIR)/sim_$(TB_NAME): $(OBJDIR)/$(TB_NAME).o $(LIBDIR)/simulator.o
	$(CC) -o $@ $^

.PHONY: clean
clean:
	rm -rf $(WORKDIRS)
