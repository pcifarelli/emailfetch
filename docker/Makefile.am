#AWS_LIBDIR = lib64
#AWS_LIBDIR = lib
AWS_LIBS = \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-core.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-s3.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-sqs.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-sns.so

EXEDEPS = ../src/emailfetch
LIBDEPS = $(AWS_LIBS)
DEPS = $(EXEDEPS) $(LIBDEPS)

noinst_PROGRAMS = docker

docker$(EXEEXT):  $(DEPS)
	cp ../Dockerfile.$(DOCKEREXT) ./Dockerfile
	mkdir -p emailfetch
	cp $(EXEDEPS) emailfetch/
	mkdir -p emailfetch/lib
	cp $(LIBDEPS) emailfetch/lib
	mkdir -p emailfetch/cfg
	cp ../cfg/* emailfetch/cfg
	/bin/bash ./docker-build.sh
	touch docker$(EXEEXT)

clean-local:
	rm -f docker$(EXEEXT)
