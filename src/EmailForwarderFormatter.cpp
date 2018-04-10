/*
 * EmailForwarderFormatter.cpp
 *
 *  Created on: Apr 7, 2018
 *      Author: Paul Cifarelli
 */

#include <unordered_set>
#include "EmailForwarderFormatter.h"

using namespace std;

EmailForwarderFormatter::EmailForwarderFormatter(string from, email_list destinations, Aws::String dir, int verbose) :
    S3Downloader::Formatter(dir, verbose),
    m_from(from), m_destinations(destinations), m_verbose(verbose)
{
}

EmailForwarderFormatter::EmailForwarderFormatter(string from, email_list destinations, Aws::String dir, mxbypref mxservers, int verbose) :
    S3Downloader::Formatter(dir, mxservers, verbose),
    m_from(from), m_destinations(destinations), m_verbose(verbose)
{
    init();
}

EmailForwarderFormatter::~EmailForwarderFormatter()
{
    init();
}

void EmailForwarderFormatter::init()
{
    // remove any duplicates
    unordered_set<string> email_set;

    for (auto &email : m_destinations)
        email_set.insert(email);

    m_destinations.clear();
    auto it = email_set.begin();
    while (it != email_set.end())
    {
        m_destinations.push_back(*it);
        it++;
    }
}

void EmailForwarderFormatter::clean_up()
{

}
