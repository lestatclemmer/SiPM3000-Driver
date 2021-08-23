# source subdirectories
IOCOND = io-containers
MAND = managers

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I. -I$(MAND) -I$(IOCOND) -I/usr/include/ -I/usr/local/include/libusb-1.0/
LDLIBS = -ldl -lusb-1.0

OUT := main
CXXNAMES := $(wildcard *.cpp) $(wildcard $(IOCOND)/*.cpp) $(wildcard $(MAND)/*.cpp)

# object files (post-compliation, pre-linking)
OBJDIR := objects
OBJS := $(addprefix $(OBJDIR)/, $(notdir $(CXXNAMES:.cpp=.o)))

# dependencies
DEPSDIR := deps
DEPENDS := $(addprefix $(DEPSDIR)/, $(notdir $(OBJS:.o=.d)))

# this is really important bc it simplifies the rules below (searching for files)
VPATH = .:$(MAND):$(IOCOND):$(OBJSDIR)

all: $(OUT)

$(OUT): depwrap $(OBJS)
	@echo 'exec out'
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(OUT) $(LDLIBS)

# wrap the dependencies to fix only once
depwrap: $(DEPENDS)
	@echo 'depwrap'
	@bash fixdeps.bash $(DEPSDIR) $(OBJDIR)

$(OBJDIR)/%.o : %.cpp | $(OBJDIR)
	@echo 'object'
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	@echo 'object directory'
	@mkdir $(OBJDIR)

.PHONY: clean
clean:
	@rm -rf $(OBJDIR) $(OUT) $(DEPSDIR) *~

print-%  : ; @echo $* = $($*)

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPENDS)
endif

$(DEPSDIR)/%.d: %.cpp | $(DEPSDIR)
	@echo 'dependencies'
	$(CXX) $(CXXFLAGS) -MM -MP -c $<>>$@

$(DEPSDIR):
	@echo 'make deps dir'
	@mkdir $(DEPSDIR)
