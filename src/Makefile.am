
# AWS_SDK_PATH is defined through an AC_CONDITION defined in configure.ac, and may be specified on the configure command line
# by setting AWSSDKPATH
#    e.g. ./configure --enable-docker AWSSDKPATH=/usr/local
# note that /usr/local is the default
#
# AWS_LIBDIR is either lib64 or lib, depending on AC_CONDITIONAL defined in configure.ac and if in top level Makefile.am

AWS_LIBS = \
	-laws-cpp-sdk-core \
	-laws-cpp-sdk-s3 \
	-laws-cpp-sdk-sqs \
	-laws-cpp-sdk-sns

# in case we are building on a UCDP machine.  If not, these do no harm
CURLINC = -I/data/ucdp/local/curl/include
CURLLIB = -L/data/ucdp/local/curl/lib

AM_CXXFLAGS = -O2 -g -DUSE_IMPORT_EXPORT -D_GNU_SOURCE -std=c++11 -I$(AWS_SDK_PATH)/include ${CURLINC}
AM_LDFLAGS = -rdynamic -Wl,-rpath,$(AWS_SDK_PATH)/${AWS_LIBDIR}

bin_PROGRAMS = emailfetch
#lib_LTLIBRARIES = libs3download.la

#libs3download_la_SOURCES = S3Get.cpp Formatter.cpp Downloader.cpp

#emailfetch_SOURCES = EmailFetcherMain.cpp MaildirFormatter.cpp MaildirDownloader.cpp
#emailfetch_LDADD = -L$(AWS_SDK_PATH)/${AWS_LIBDIR} -ls3download $(AWS_LIBS)
emailfetch_SOURCES = EmailFetcherMain.cpp \
	EmailFetcherConfig.cpp \
	UCDPFormatter.cpp \
	EmailExtractor.cpp \
	MaildirFormatter.cpp \
	EmailForwarderFormatter.cpp \
	S3Get.cpp \
	Formatter.cpp \
	Downloader.cpp \
	SMTPSender.cpp \
	UCDPCurlPoster.cpp

emailfetch_LDADD = ${CURLLIB} -L$(AWS_SDK_PATH)/${AWS_LIBDIR} $(AWS_LIBS)

