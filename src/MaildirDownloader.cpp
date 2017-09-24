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

mode_t MaildirDownloader::m_newdirs_mode = 0700;

MaildirDownloader::MaildirDownloader(const Aws::String dir, const int days, Aws::String topic_arn, Aws::String bucket_name, MaildirFormatter &fmt) :
    m_dir(dir), m_curdir(dir + "/cur"), m_tmpdir(dir + "/tmp"), m_newdir(dir + "/new"), m_fmt(fmt),
    Downloader(dir + "/cur", days, topic_arn, bucket_name, fmt)
{
    mkdirs(m_curdir.c_str(), m_newdirs_mode);
    mkdirs(m_newdir.c_str(), m_newdirs_mode);
    mkdirs(m_tmpdir.c_str(), m_newdirs_mode);
}

MaildirDownloader::~MaildirDownloader()
{
}

int MaildirDownloader::mkdirs(std::string dirname, mode_t mode)
{
    std::istringstream iss(dirname);
    std::string path = "";
    struct stat fstats;

    if (dirname.at(0) == '/')
        path = "/";

    std::string s;
    while (std::getline(iss, s, '/'))
    {
        if (s != "")
        {
            path += s;
            if (!stat(path.c_str(), &fstats))
            {
                if (S_ISDIR(fstats.st_mode))
                    path += "/";
                else
                {
                    std::cout << "ERROR: " << path << " exists but is not a dir" << std::endl;
                    return -1;
                }

            }
            else
            {
                path += "/";
                if (mkdir(path.c_str(), mode))
                {
                    char err[512], *p;
                    p = strerror_r(errno, err, 512);
                    std::cout << "ERROR: mkdir failed " << err << std::endl;
                    return -1;
                }
            }
        }
    }
    return 0;
}

