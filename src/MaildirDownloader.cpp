/*
 * MaildirDownloader.cpp
 *
 *  Created on: Sep 24, 2017
 *      Author: paulc
 */

#include <dirent.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include "MaildirDownloader.h"

MaildirDownloader::MaildirDownloader(const Aws::String dir, const int days, Aws::String topic_arn, Aws::String bucket_name,
    MaildirFormatter &fmt) :
    Downloader(dir + "/cur", days, topic_arn, bucket_name, fmt)
{
}

MaildirDownloader::~MaildirDownloader()
{
}

