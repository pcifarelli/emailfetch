AWS_SDK_PATH=${HOME}/develop/aws-sdk-cpp
#AWS_SDK_PATH=${HOME}/develop/ucdp_core_bins/local/aws-sdk-cpp
#AWS_SDK_PATH=/usr/local
#AWS_LIBDIR = lib64
AWS_LIBDIR = lib
AWS_LIBS = \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-core.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-s3.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-sqs.so \
	$(AWS_SDK_PATH)/$(AWS_LIBDIR)/libaws-cpp-sdk-sns.so

noinst_PROGRAMS = docker

docker$(EXEEXT):  ../src/emailfetch $(AWS_LIBS)
	sh ./docker-build.sh
	touch docker$(EXEEXT)

clean-local:
	rm -f docker$(EXEEXT)
