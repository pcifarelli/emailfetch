/*
 * EmailForwarderFormatter.cpp
 *
 *  Created on: Apr 7, 2018
 *      Author: Paul Cifarelli
 */

#include <locale.h>
#include <unordered_set>
#include "MaildirFormatter.h"
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

    // set the locale up for date
    setlocale(LC_TIME, "");
}

void EmailForwarderFormatter::clean_up()
{
    string fullpath = m_fullpath.c_str();
    string formattedFile = getSaveDir().c_str();
    formattedFile +=  ".";
    formattedFile += getName().c_str();

    for (auto &eaddr : m_destinations)
    {
        formatFile(eaddr, fullpath, formattedFile);
        forwardFile(eaddr, formattedFile);
    }

    remove(formattedFile.c_str());
}

void EmailForwarderFormatter::formatFile(string to, string fullpath, string formattedFile)
{
    string md, o_msgid, o_to, o_from, o_subject, o_date, dummy;
    string contenttype;
    EmailExtractor::ToSet dummyset;
    string email = "Date: " + date() + "\r\n";
    email += "To: " + to + "\r\n";
    email += "From: " + m_from + "\r\n";

    EmailExtractor::scan_headers(fullpath, o_msgid, o_to, o_from, o_subject, o_date, dummy, dummy, dummy, dummy, dummy, dummyset, dummyset, dummyset);
    if (MaildirFormatter::sha256_as_str((void *)o_msgid.c_str(), o_msgid.length(), md))
        email += "Message-ID: <" + md + "@fastemail.tr>" + "\r\n";
    else
        email += "Message-ID: <" + o_msgid + "@fastemail.tr>" + "\r\n";
    email += "References: <" + o_msgid + ">" + "\r\n";

    email += "Subject: FWD: " + o_subject + "\r\n";
    email += "\r\n";
    email += "------------------ Original Message ------------------\r\n";

    ifstream infile(fullpath, ifstream::in);
    string str;

    infile.seekg(0, std::ios::end);
    str.reserve(infile.tellg());
    infile.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());

    email += str;
    cout << "---------------------------------------------------------------------------------" << endl;
    cout << email << endl;
    cout << "---------------------------------------------------------------------------------" << endl;
}

void EmailForwarderFormatter::forwardFile(string to, string formattedFile)
{

}

string EmailForwarderFormatter::date()
{
    time_t current_time;
    char rfc_2822[40];

    time(&current_time);
    strftime(
            rfc_2822,
            sizeof(rfc_2822),
            "%a, %d %b %Y %T %z",
            localtime(&current_time)
            );
    string ret = rfc_2822;
    return ret;
}

