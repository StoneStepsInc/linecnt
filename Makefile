.SUFFIXES:

SRCS = linecnt.cpp cpplexer_imp.cpp
OBJS = $(SRCS:.cpp=.o)
LEXERS = cpplexer.l
LEXERO = $(LEXERS:.l=.inc)
TARGET = linecnt

#
#
#

CCFLAGS = $(CFLAGS) $(CPPFLAGS) -fexceptions

ifeq ($(findstring -g,$(CFLAGS)),)
CFLAGS += -O2
endif

#
#
#
define clean-target
	@for obj in $(2); do if [[ -e $$obj ]]; then rm $$obj; fi; done
	@if [[ -e $(1) ]]; then rm $(1); fi
endef

#

define add-depend
	sed 's/\($(2)\).o[ :]*/\1.o $(1) : /g' > $(1); \
	[ -s $(1) ] || rm -f $(1)
endef

#
#
%.d : %.cpp
	set -e; $(CC) -M $(CCFLAGS) $(CXXFLAGS) $(INCLUDES) $< | \
		$(call add-depend,$@,$*)

%.d : %.c
	set -e; $(CC) -M $(CCFLAGS) $(INCLUDES) $< | \
		$(call add-depend,$@,$*)

%.o : %.c
	$(CC) -c -o $@ $(CCFLAGS) $<

%.o : %.cpp
	$(CC) -c -o $@ $(CCFLAGS) $(CXXFLAGS) $<

%.inc : %.l
	$(LEX) -o$@ $<

#
#
#
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ -lstdc++

clean:
	$(call clean-target,$(TARGET),$(OBJS))
	$(call clean-target,no-such-file,$(LEXERO))


#
#
#

include $(OBJS:.o=.d)

