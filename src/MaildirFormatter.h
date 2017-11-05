/*
 * MaildirFormatter.h
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#ifndef SRC_MAILDIRFORMATTER_H_
#define SRC_MAILDIRFORMATTER_H_

#include "Formatter.h"
#include <aws/core/Aws.h>
#include <string>
#include <cstddef>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <errno.h>

class MaildirFormatter: public S3Downloader::Formatter
{
public:
    MaildirFormatter(Aws::String dir);
    virtual ~MaildirFormatter();

    // open the file
    virtual void open(const Aws::S3::Model::Object obj, const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    // makes the name of the file for this format
    virtual std::string mkname(const Aws::S3::Model::Object obj) const;
    // gets the S3 key from the filename (extracts the SHA hash for this format)
    virtual Aws::String getKey(Aws::String filename) const;

    virtual Aws::String &getSaveDir()  { return m_tmpdir; }
    virtual Aws::String &getTrackDir() { return m_curdir; }

    static bool SHA256(void* input, unsigned long length, unsigned char* md);
    static bool sha256_as_str(void *input, unsigned long length, std::string &md);
    void close_and_rename();
    void clean_up();

private:
    int construct_name(Aws::S3::Model::Object obj, Aws::String key, std::string &name) const;
    static int extractHash(Aws::String name, Aws::String &hash);

    std::string         m_host;
    pid_t               m_pid;
    static unsigned int m_q_sequence;
    Aws::String         m_newpath;
    bool                m_isopen;

protected:
    Aws::String       m_curdir;
    Aws::String       m_tmpdir;
    Aws::String       m_newdir;
};

#endif /* SRC_MAILDIRFORMATTER_H_ */
