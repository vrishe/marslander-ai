my_output_kind ?= exe
ifeq ($(findstring $(my_output_kind),exe lib),)
$(error Unsupported my_output_kind: '$(my_output_kind)')
endif

output_name := $(notdir $(CURDIR))

build_dir := ../$(BUILD_DIR)
gen_base_dir := $(build_dir)/__gen

src_dirs := ./
inc_dirs := ../include $(shell find $(src_dirs) -type d) $(gen_base_dir)/$(output_name)\
	$(my_include_dirs)

build_dir := $(build_dir)/$(BUILD_CONFIG)
gen_dir := $(gen_base_dir)/$(output_name)
lib_dir := ../lib
out_dir := $(build_dir)/obj/$(output_name)

pbs := $(shell find $(src_dirs) -name '*.proto')
srcs := $(shell find $(src_dirs) -name '*.cpp')

gen_protoc := $(pbs:%.proto=$(gen_dir)/%.pb.cc)

objs += $(pbs:%.proto=$(out_dir)/%.pb.cc.o)
objs += $(srcs:%=$(out_dir)/%.o)

-include $(pbs:%=$(gen_dir)/%.d)
-include $(objs:.o=.d)

# EXE =========================================================================

ifeq (exe, $(my_output_kind))
LDFLAGS := -L$(lib_dir) -L$(build_dir)
LDLIBS := $(my_libs) -lstdc++ -lm

exe := $(build_dir)/$(output_name)
exe_preq := $(gen_protoc) $(objs)

ifneq (,$(my_modules))
inc_dirs += $(my_modules:%=../%) $(my_modules:%=$(gen_base_dir)/%)
LDLIBS := $(my_modules:%=-lmod%) $(LDLIBS)

mods := $(my_modules:%=$(build_dir)/libmod%.a)
exe_preq := $(mods) $(exe_preq)

.PHONY: $(my_modules)
$(my_modules):
	$(MAKE) -C "$(ROOT_DIR)/$@";

.DEFAULT_GOAL := $(output_name)_proxy
.PHONY: $(DEFAULT_GOAL)
$(.DEFAULT_GOAL): $(my_modules) $(exe)
else
.DEFAULT_GOAL := $(exe)
endif # mods

$(exe) : $(exe_preq)
	@echo $(my_output_kind)
	$(CC) $(objs) -o $@ $(LDFLAGS) $(LDLIBS)
endif # exe

# LIB =========================================================================

ifeq (lib,$(my_output_kind))
ARFLAGS := rcsv

lib_preq := $(gen_protoc) $(objs)

.DEFAULT_GOAL := $(build_dir)/libmod$(output_name).a
$(.DEFAULT_GOAL) : $(lib_preq)
	@echo $(my_output_kind)
	$(AR) $(ARFLAGS) $@ $(objs)
endif # lib

# OTHER =======================================================================

CPPFLAGS.debug := 
CPPFLAGS.release := -DNDEBUG
CPPFLAGS += -MMD -MP $(CPPFLAGS.$(BUILD_CONFIG))
CXXFLAGS.debug := -ggdb3
CXXFLAGS.release := -O2 -s
CXXFLAGS += -std=c++17 -Wpedantic -Werror $(CXXFLAGS.$(BUILD_CONFIG))

$(out_dir)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(addprefix -I,$(inc_dirs)) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(out_dir)/%.pb.cc.o: $(gen_dir)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(addprefix -I,$(shell find $(gen_dir) -type d)) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PRECIOUS: $(gen_dir)/%.pb.cc
$(gen_dir)/%.pb.cc: %.proto
	@mkdir -p $(dir $@)
	protoc --cpp_out=$(gen_dir) --dependency_out=$(gen_dir)/$<.d $<

# uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))
