my_include_dirs += ../3rd-party/googletest/googletest/include \
	../3rd-party/json/include ../3rd-party/sockpp/include
my_libs += -lgtest -lgtest_main -lsockpp -lprotobuf
my_modules += crc32 shared base64

include ../Module.mk

.PHONY: run
run: $(.DEFAULT_GOAL)
	$(exe)
