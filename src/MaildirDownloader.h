/*
 * MaildirDownloader.h
 *
 *  Created on: Sep 24, 2017
 *      Author: paulc
 */

#ifndef SRC_MAILDIRDOWNLOADER_H_
#define SRC_MAILDIRDOWNLOADER_H_

#include "Downloader.h"
#include "MaildirFormatter.h"
using namespace S3Downloader;

class MaildirDownloader: public S3Downloader::Downloader
{
public:
    MaildirDownloader(const Aws::String dir, const int days, Aws::String topic_arn, Aws::String bucket_name, MaildirFormatter &fmt);
    virtual ~MaildirDownloader();

    virtual Aws::String &getSaveDir() { return m_dir; }

private:
    Aws::String       m_dir;
    MaildirFormatter &m_fmt;
 };

#endif /* SRC_MAILDIRDOWNLOADER_H_ */
