/*
 * MaildirFormatter.cpp
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#include "MaildirFormatter.h"
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

unsigned int MaildirFormatter::m_q_sequence = 0;
//unsigned int MaildirFormatter::m_ur = 0;

MaildirFormatter::MaildirFormatter() : Formatter()
{
    char host[NI_MAXHOST], *p;

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

    //if (!(m_fd = ::open("/dev/urandom", O_RDONLY | O_NONBLOCK)))
    //    std::cerr << "Unable to get random numbers" << std::endl;
}

MaildirFormatter::~MaildirFormatter()
{
    //::close(m_fd);
}

bool MaildirFormatter::SHA256(void* input, unsigned long length, unsigned char* md)
{
    SHA256_CTX context;
    if(!SHA256_Init(&context))
        return false;

    if(!SHA256_Update(&context, (unsigned char*)input, length))
        return false;

    if(!SHA256_Final(md, &context))
        return false;

    return true;
}

bool MaildirFormatter::sha256_as_str(void *input, unsigned long length, std::string &mds)
{
    unsigned char md[SHA256_DIGEST_LENGTH]; // 32 bytes
    if(!SHA256(input, length, md))
        return false;

    std::ostringstream out;
    char buf[3];
    for (int i=0; i<32; i++)
    {
        snprintf(buf, sizeof(buf), "%02x", md[i]);
        out << buf;
    }
    mds = out.str();
    return true;
}

int MaildirFormatter::mkname(Aws::String key, std::string &name)
{
    struct timeval tv;
    struct timezone tz;
    char usecs[7];

    if (gettimeofday(&tv, &tz))
    {
        // not much we can do here if we cant get the time.  something bad happened to the system.
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        std::cerr << "Something bad has happened; can't get time of day from the system" << std::endl;
    }

    snprintf(usecs, sizeof(usecs), "%06ld", tv.tv_usec);

    // increment the Q for this process
    ++m_q_sequence;

    //if (!(::read(m_fd, (void *) &m_ur, (size_t) sizeof(m_ur)) == sizeof(m_ur)))
    //    std::cerr << "unable to get random number: " << m_ur << std::endl;

    // use the SHA256 of the object name as the random number
    std::string sha;
    if(!sha256_as_str((void *) key.c_str(), key.length(), sha))
    {
        std::cerr << "Unable to get hash of object key " << key << std::endl;
        return -1;
    }

    std::ostringstream out;
    //out << tv.tv_sec << ".P" << m_pid << "Q" << m_q_sequence << "M" << tv.tv_usec << "R" << m_ur << '.' << m_host;
    out << tv.tv_sec << ".P" << m_pid << "Q" << m_q_sequence << "M" << usecs << "R" << sha << '.' << m_host;

    name = out.str();
    return 0;
}

int MaildirFormatter::extractHash(std::string name, std::string &hash)
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

void MaildirFormatter::open(const Aws::String pathname, const Aws::String name, const std::ios_base::openmode mode)
{
    std::string fname;

    if (mkname(name, fname))
        std::cerr << "Unable to construct usable name" << std::endl;

    std::string pname = pathname.c_str();
    std::size_t n = pname.find_last_of('/');
    if (n != std::string::npos && n != pname.length() - 1)
        pname += '/';

    Formatter::open(pname.c_str(), fname.c_str(), mode);
}

