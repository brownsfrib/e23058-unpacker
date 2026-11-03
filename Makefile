# combined-unpacker Makefile
#
#   make                  build bin/unpacker from unpacker.cxx
#   make MAIN=channels    build bin/channels from channels.cxx (any top-level .cxx)
#   make -j8              parallel build
#   make clean            remove objects and binaries
#   make distclean        also remove obj/ and bin/
#
# Every .cpp in SRCDIRS is compiled and linked automatically. To add a class,
# drop its .cpp in src/ or modules/src/ and its .h in one of INCDIRS.

CXX      := g++
OBJDIR   := obj
MAIN     ?= unpacker
TARGET   := bin/$(MAIN)

ROOTCFLAGS := $(shell root-config --cflags)
ROOTLIBS   := $(shell root-config --glibs)

INCDIRS  := include modules/include modules/detector-specific
SRCDIRS  := src modules/src modules/detector-specific/src

CXXFLAGS := -O2 -Wall $(ROOTCFLAGS)
CPPFLAGS := $(addprefix -I,$(INCDIRS))
DEPFLAGS := -MMD -MP
LDFLAGS  := $(ROOTLIBS)

SOURCES  := $(foreach d,$(SRCDIRS),$(wildcard $(d)/*.cpp))
OBJECTS  := $(OBJDIR)/$(MAIN).o $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.cpp=.o)))
DEPS     := $(OBJECTS:.o=.d)
vpath %.cpp $(SRCDIRS)

# ANSI colors
GREEN  := \033[0;32m
CYAN   := \033[0;36m
YELLOW := \033[0;33m
BOLD   := \033[1m
RESET  := \033[0m

# Progress: TOTAL = files that actually need compiling this time (found with a
# dry run), so a one-file rebuild still counts up to 100%.
ifndef DRYRUN
TOTAL := $(shell $(MAKE) --no-print-directory -n DRYRUN=1 MAIN=$(MAIN) $(MAKECMDGOALS) 2>/dev/null | grep -c "@@STEP@@")
endif
STEP := 0
define advance
$(eval STEP := $(shell echo $$(( $(STEP) + 1 ))))
endef
define pct
$(shell echo $$(( $(1) * 100 / ($(2) + 0 > 0 ? $(2) : 1) )))
endef

.PHONY: all clean distclean

all: $(TARGET)
	@printf "\n$(GREEN)$(BOLD)Build complete: $(RESET)$(TARGET)\n\n"

$(TARGET): $(OBJECTS) | bin
	@printf "$(CYAN)$(BOLD)[100%%]$(RESET)  Linking $(BOLD)$(TARGET)$(RESET)\n"
	@$(CXX) $^ -o $@ $(LDFLAGS)

$(OBJDIR) bin:
	@mkdir -p $@

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(call advance)
	@printf "$(GREEN)[%3d%%]$(RESET)  Building class $<\n" "$(call pct,$(STEP),$(TOTAL))" # @@STEP@@
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJDIR)/$(MAIN).o: $(MAIN).cxx | $(OBJDIR)
	$(call advance)
	@printf "$(GREEN)[%3d%%]$(RESET)  Building executable $<\n" "$(call pct,$(STEP),$(TOTAL))" # @@STEP@@
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	@printf "$(YELLOW)Cleaning build artifacts$(RESET)\n"
	@rm -f $(OBJDIR)/*.o $(OBJDIR)/*.d $(addprefix bin/,$(basename $(wildcard *.cxx)))

distclean: clean
	@printf "$(YELLOW)Removing build directories...$(RESET)\n"
	@rm -rf $(OBJDIR) bin

-include $(DEPS)
