AWS_SDK_PATH=${HOME}/develop/aws-sdk-cpp
AWS_LIBS = \
	-laws-cpp-sdk-core \
	-laws-cpp-sdk-s3

AM_CXXFLAGS = -O2 -DUSE_IMPORT_EXPORT -std=c++11 -I$(AWS_SDK_PATH)/include
AM_LDFLAGS = -rdynamic -Wl,-rpath,$(AWS_SDK_PATH)/lib

bin_PROGRAMS = emailfetch
emailfetch_SOURCES = EmailFetcherMain.cpp S3Get.cpp Formatter.cpp MaildirFormatter.cpp Downloader.cpp
emailfetch_LDADD = -L$(AWS_SDK_PATH)/lib $(AWS_LIBS)
