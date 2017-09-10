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

class MaildirFormatter: public Formatter
{
public:
    MaildirFormatter();
    virtual ~MaildirFormatter();
    virtual std::string mkname(const Aws::S3::Model::Object obj) const;
    virtual Aws::String getKey(Aws::String filename) const;

    static int extractHash(Aws::String name, Aws::String &hash);

    static bool SHA256(void* input, unsigned long length, unsigned char* md);
    static bool sha256_as_str(void *input, unsigned long length, std::string &md);

private:
    int construct_name(Aws::S3::Model::Object obj, Aws::String key, std::string &name) const;

    std::string         m_host;
    pid_t               m_pid;
    static unsigned int m_q_sequence;
};

#endif /* SRC_MAILDIRFORMATTER_H_ */
