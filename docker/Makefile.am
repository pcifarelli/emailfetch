
# AWS_SDK_PATH is defined through an AC_CONDITION defined in configure.ac, and may be specified on the configure command line
# by setting AWSSDKPATH
#    e.g. ./configure --enable-docker AWSSDKPATH=/usr/local
# note that /usr/local is the default
#
# AWS_LIBDIR is either lib64 or lib, depending on AC_CONDITIONAL defined in configure.ac and if in top level Makefile.am

AWS_LIBS = \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-core.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-s3.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-sqs.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-sns.so

EXEDEPS = ../src/emailfetch
LIBDEPS = $(AWS_LIBS)
DEPS = $(EXEDEPS) $(LIBDEPS)

noinst_PROGRAMS = docker$(EXEEXT)

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
	rm -rf emailfetch
