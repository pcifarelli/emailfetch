/*
 * EmailForwarderFormatter.cpp
 *
 *  Created on: Apr 7, 2018
 *      Author: Paul Cifarelli
 */

#include "EmailForwarderFormatter.h"

using namespace std;

EmailForwarderFormatter::EmailForwarderFormatter(Aws::String to, Aws::String from, Aws::String outgoingserver, int verbose) :
    S3Downloader::Formatter(verbose), m_to(to.c_str()), m_from(from.c_str()), m_outgoingserver(outgoingserver.c_str()), m_verbose(verbose)
{
}

EmailForwarderFormatter::EmailForwarderFormatter(Aws::String to, Aws::String from, Aws::String outgoingserver, mxbypref mxservers, int verbose) :
    S3Downloader::Formatter(mxservers, verbose), m_to(to.c_str()), m_from(from.c_str()), m_outgoingserver(outgoingserver.c_str()), m_verbose(verbose)
{
}

EmailForwarderFormatter::EmailForwarderFormatter(Aws::String to, Aws::String from, Aws::String outgoingserver, Aws::String dir, int verbose) :
    S3Downloader::Formatter(dir, verbose), m_to(to.c_str()), m_from(from.c_str()), m_outgoingserver(outgoingserver.c_str()), m_verbose(verbose)
{
}

EmailForwarderFormatter::EmailForwarderFormatter(Aws::String to, Aws::String from, Aws::String outgoingserver, Aws::String dir, mxbypref mxservers, int verbose) :
    S3Downloader::Formatter(dir, mxservers, verbose), m_to(to.c_str()), m_from(from.c_str()), m_outgoingserver(outgoingserver.c_str()), m_verbose(verbose)
{
}

EmailForwarderFormatter::~EmailForwarderFormatter()
{
}

void EmailForwarderFormatter::clean_up()
{

}
