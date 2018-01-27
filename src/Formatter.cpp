/*
 * Formatter.cpp
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 *
 *      This simple formatter just saves each file using the object name as the file
 */

#include <dirent.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <aws/core/Aws.h>
#include <string>
#include <cstddef>
#include <climits>
#include <fcntl.h>
#include "Formatter.h"
#include "SMTPSender.h"

namespace S3Downloader
{

mode_t Formatter::m_newdirs_mode = 0700;

Formatter::Formatter(int verbose) :
    m_forward(false), m_dir(""), m_local_file(NULL), m_fullpath(""), m_verbose(verbose)
{
}

Formatter::Formatter(Aws::String dir, int verbose) :
    m_forward(false), m_dir(dir), m_local_file(NULL), m_fullpath(""), m_verbose(verbose)
{
    mkdirs(m_dir.c_str(), m_newdirs_mode);
}

Formatter::Formatter(std::vector<std::string> mxservers, int verbose) :
    m_forward(true), m_mxservers(mxservers), m_dir(""), m_local_file(NULL), m_fullpath(""), m_verbose(verbose)
{
    if (!m_mxservers.size())
        m_forward = false; // sorry, can't forward if there are no mxservers
}

Formatter::Formatter(Aws::String dir, std::vector<std::string> mxservers, int verbose) :
    m_forward(true), m_mxservers(mxservers), m_dir(dir), m_local_file(NULL), m_fullpath(""), m_verbose(verbose)
{
    if (!m_mxservers.size())
        m_forward = false; // sorry, can't forward if there are no mxservers

    mkdirs(m_dir.c_str(), m_newdirs_mode);
}

Formatter::~Formatter()
{
    close();
}

// Pick 1 random MX server from the list and forward the file to that server
void Formatter::do_forwarding(std::string fullpath)
{
    if (m_forward)
    {
        int server_number = random_number() * m_mxservers.size();
        std::string server = m_mxservers[server_number];
        SMTPSender smtpsender(server);
        if (m_verbose >=3)
            std::cout << "Forwarding " << fullpath << " to MX server " << server << std::endl;

        smtpsender.forwardFile(fullpath);
        if (smtpsender.status() != CURLE_OK)
            std::cout << "Error forwarding " << fullpath << " to MX server " << server << std::endl;
    }
}

// Pick 1 random MX server from the list and forward the file to that server
void Formatter::do_forwarding(std::string fullpath, std::string to)
{
    if (m_forward)
    {
        int server_number = random_number() * m_mxservers.size();
        std::string server = m_mxservers[server_number];
        SMTPSender smtpsender(server);
        if (m_verbose >=3)
            std::cout << "Forwarding " << fullpath << " to MX server " << server << std::endl;

        smtpsender.forwardFile(fullpath, to);
        if (smtpsender.status() != CURLE_OK)
            std::cout << "Error forwarding " << fullpath << " to MX server " << server << std::endl;
    }
}

double Formatter::random_number()
{
    unsigned long ur;
    int fd;
    if (!(fd = ::open("/dev/urandom", O_RDONLY | O_NONBLOCK)))
        std::cout << "Unable to get random number" << std::endl;
    else
    {
        if (::read(fd, (void *) &ur, sizeof(ur)) != sizeof(ur))
            std::cout << "Could not read a whole random number: " << ur << std::endl;
        ::close(fd);
    }

    return (double) ur / (double) ULONG_MAX;
}


void Formatter::close()
{
    if (m_local_file.is_open())
        m_local_file.close();
}

std::string Formatter::mkname(const Aws::S3::Model::Object obj) const
{
    return obj.GetKey().c_str();
}

void Formatter::open(const Aws::S3::Model::Object obj, const std::ios_base::openmode mode)
{
    Aws::String pathname = getSaveDir();
    std::string name = mkname(obj);
    Aws::String filename = name.c_str();

    Aws::String c = "";
    close();

    std::size_t n = pathname.find_last_of('/');
    if (n != std::string::npos && n != pathname.length() - 1)
        c = "/";

    m_fullpath = pathname + c + filename;

    m_name = filename;
    m_local_file.open(m_fullpath.c_str(), mode);
}

Aws::OStream &Formatter::getStream(void *buf, size_t sz)
{
    m_ostream.rdbuf()->pubsetbuf((char *) buf, sz);
    return m_ostream;
}

Aws::String Formatter::getKey(Aws::String filename) const
{
    return filename;
}

int Formatter::mkdirs(std::string dirname, mode_t mode)
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

}

