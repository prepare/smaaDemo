sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


FILES:= \
	# empty line


smaaDemo_MODULES:=imgui renderer sdl2 shaderc spirv-cross utils
smaaDemo_SRC:=$(foreach f, smaaDemo.cpp, $(dir)/$(f))


PROGRAMS+= \
	smaaDemo \
	# empty line

SRC_$(d):=$(addprefix $(d)/,$(FILES))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
