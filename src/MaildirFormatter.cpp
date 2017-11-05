/*
 * MaildirFormatter.cpp
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "MaildirFormatter.h"

unsigned int MaildirFormatter::m_q_sequence = 0;

MaildirFormatter::MaildirFormatter(Aws::String dir) : Formatter(dir), m_isopen(false)
{
    char host[NI_MAXHOST];

    if (!gethostname(host, sizeof(host)))
        m_host = host;
    else
        m_host = "dummy";

    int pos;
    while ((pos = m_host.find_first_of('/')) != std::string::npos)
        m_host.replace(pos, 1, 1, '_');
    while ((pos = m_host.find_first_of(':')) != std::string::npos)
        m_host.replace(pos, 1, 1, '_');

    m_pid = getpid();

    Aws::String c = "";
    std::size_t n = dir.find_last_of('/');
    if (n != std::string::npos && n != dir.length() - 1)
        c = "/";
    m_curdir = dir + c + "cur/";
    m_newdir = dir + c + "new/";
    m_tmpdir = dir + c + "tmp/";

    mkdirs(m_curdir.c_str(), m_newdirs_mode);
    mkdirs(m_newdir.c_str(), m_newdirs_mode);
    mkdirs(m_tmpdir.c_str(), m_newdirs_mode);
}

MaildirFormatter::~MaildirFormatter()
{
    close_and_rename();
}

void MaildirFormatter::close_and_rename()
{
    Aws::String newpath = m_newdir + m_name;
    Aws::String tmppath = m_tmpdir + m_name;

    if (m_isopen)
    {
        if (::rename(tmppath.c_str(), newpath.c_str()))
        {
            int lerrno = errno;
            char buf[1024], *p;
            if (lerrno && (p = strerror_r(lerrno, buf, 1024)))
                std::cout << "Error: Failed to rename file " << m_name << " to maildir new directory (" << p << ")"  << std::endl;
            else
                std::cout << "Error: Failed to rename file " << m_name << " to maildir new directory (" << lerrno << ")"  << std::endl;

            std::cout << "  From: " << tmppath << std::endl;
            std::cout << "  To:   " << newpath << std::endl;
        }
    }
    Formatter::close();
    m_isopen = false;
}

void MaildirFormatter::open(const Aws::S3::Model::Object obj, const std::ios_base::openmode mode)
{
    std::string name = mkname(obj);
    std::string fullpath = m_tmpdir.c_str();
    fullpath += name;

    m_name = name.c_str();
    m_local_file.open(fullpath.c_str(), mode);
    m_isopen = true;
}

void MaildirFormatter::clean_up()
{
    close_and_rename();
}

Aws::String MaildirFormatter::getKey(Aws::String filename) const
{
    Aws::String hash;
    extractHash(filename, hash);
    return hash;
}

bool MaildirFormatter::SHA256(void* input, unsigned long length, unsigned char* md)
{
    SHA256_CTX context;
    if (!SHA256_Init(&context))
        return false;

    if (!SHA256_Update(&context, (unsigned char*) input, length))
        return false;

    if (!SHA256_Final(md, &context))
        return false;

    return true;
}

bool MaildirFormatter::sha256_as_str(void *input, unsigned long length, std::string &mds)
{
    unsigned char md[SHA256_DIGEST_LENGTH]; // 32 bytes
    if (!SHA256(input, length, md))
        return false;

    std::ostringstream out;
    char buf[3];
    for (int i = 0; i < 32; i++)
    {
        snprintf(buf, sizeof(buf), "%02x", md[i]);
        out << buf;
    }
    mds = out.str();
    return true;
}

int MaildirFormatter::construct_name(Aws::S3::Model::Object obj, Aws::String key, std::string &name) const
{
    struct timeval tv =
    { 0, 0 };

    std::chrono::system_clock::time_point t = obj.GetLastModified().UnderlyingTimestamp();
    std::time_t ts = std::chrono::system_clock::to_time_t(t);
    std::chrono::system_clock::time_point ts_m = std::chrono::system_clock::from_time_t(ts);

    tv.tv_sec = ts;
    tv.tv_usec = std::chrono::duration_cast < std::chrono::microseconds > (t - ts_m).count();
    //std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t - ts_m).count() << std::endl;

    //std::cout << "Key: " << obj.GetKey() << std::endl;
    //std::cout << "Size: " << obj.GetSize() << std::endl;
    //std::cout << "LastModified: " << obj.GetLastModified().ToGmtString("%F %T") << std::endl;
    //std::cout << "LastModified TS: " << ts << std::endl;
    //std::cout << "ETag: " << obj.GetETag() << std::endl;

    //snprintf(usecs, sizeof(usecs), "%06ld", tv.tv_usec);
    //std::cout << tv.tv_usec << "   " << usecs << std::endl;

    // increment the Q for this process
    ++m_q_sequence;

    // use the SHA256 of the object name as the random number
    std::string sha;
    if (!sha256_as_str((void *) key.c_str(), key.length(), sha))
    {
        std::cerr << "Unable to get hash of object key " << key << std::endl;
        return -1;
    }

    std::ostringstream out;
    out << tv.tv_sec << ".P" << m_pid << "Q" << m_q_sequence << "R" << sha << '.' << m_host;

    name = out.str();
    return 0;
}

int MaildirFormatter::extractHash(Aws::String name, Aws::String &hash)
{
    int posf = name.find_first_of('.');
    if (posf == std::string::npos)
        return -1;

    int posl = name.find_last_of('.');
    if (posl == std::string::npos)
        return -1; // invalid name

    int posh = name.find_first_of('R');
    if (posh == std::string::npos)
        return -1;

    hash = name.substr(posh + 1, posl - posh - 1);

    return 0;
}

std::string MaildirFormatter::mkname(const Aws::S3::Model::Object obj) const
{
    Aws::String name = obj.GetKey();
    std::string fname;

    if (construct_name(obj, name, fname))
        std::cerr << "Unable to construct usable name" << std::endl;

    return fname;
}

