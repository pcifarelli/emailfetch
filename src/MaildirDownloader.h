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

    class MaildirDestination : public Maildestination
    {
    public:
        Aws::String       m_dir;
        Aws::String       m_curdir;
        Aws::String       m_tmpdir;
        Aws::String       m_newdir;
    };

private:
    static int mkdirs(std::string dirname, mode_t mode);
    static mode_t     m_newdirs_mode;
};

#endif /* SRC_MAILDIRDOWNLOADER_H_ */
