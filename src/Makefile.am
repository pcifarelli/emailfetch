#AWS_SDK_PATH=${HOME}/develop/aws-sdk-cpp
#AWS_SDK_PATH=${HOME}/develop/ucdp_core_bins/local/aws-sdk-cpp
AWS_SDK_PATH=/usr/local
AWS_LIBDIR = lib64
#AWS_LIBDIR = lib
AWS_LIBS = \
	-laws-cpp-sdk-core \
	-laws-cpp-sdk-s3 \
	-laws-cpp-sdk-sqs \
	-laws-cpp-sdk-sns

AM_CXXFLAGS = -O2 -g -DUSE_IMPORT_EXPORT -std=c++11 -I$(AWS_SDK_PATH)/include
AM_LDFLAGS = -rdynamic -Wl,-rpath,$(AWS_SDK_PATH)/${AWS_LIBDIR}

bin_PROGRAMS = emailfetch
#lib_LTLIBRARIES = libs3download.la

#libs3download_la_SOURCES = S3Get.cpp Formatter.cpp Downloader.cpp

#emailfetch_SOURCES = EmailFetcherMain.cpp MaildirFormatter.cpp MaildirDownloader.cpp
#emailfetch_LDADD = -L$(AWS_SDK_PATH)/${AWS_LIBDIR} -ls3download $(AWS_LIBS)
emailfetch_SOURCES = EmailFetcherMain.cpp \
	MaildirFormatter.cpp \
	MaildirDownloader.cpp \
	S3Get.cpp \
	Formatter.cpp \
	Downloader.cpp

emailfetch_LDADD = -L$(AWS_SDK_PATH)/${AWS_LIBDIR} $(AWS_LIBS)

