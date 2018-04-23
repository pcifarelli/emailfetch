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

EmailForwarderFormatter::EmailForwarderFormatter(string from, email_list destinations, outgoing_mail_server server_info, Aws::String dir, int verbose) :
    S3Downloader::Formatter(dir, verbose),
    m_domain(""), m_from(from), m_destinations(destinations), m_server_info(server_info), m_verbose(verbose)
{
    init();
}

EmailForwarderFormatter::EmailForwarderFormatter(string from, email_list destinations, outgoing_mail_server server_info, Aws::String dir, mxbypref mxservers, int verbose) :
    S3Downloader::Formatter(dir, mxservers, verbose),
    m_domain(""), m_from(from), m_destinations(destinations), m_server_info(server_info), m_verbose(verbose)
{
    init();
}

EmailForwarderFormatter::~EmailForwarderFormatter()
{
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

    std::size_t n = m_from.find_last_of('@');
    if (n != std::string::npos)
        m_domain = m_from.substr(n+1);

    if (m_verbose > 3)
        cout << "DOMAIN is " << m_domain << endl;
}

void EmailForwarderFormatter::clean_up()
{
    string fullpath = m_fullpath.c_str();
    string formattedFile = getSaveDir().c_str();
    formattedFile +=  ".";
    formattedFile += getName().c_str();

    for (auto &eaddr : m_destinations)
    {
        string email = formatFile(eaddr, fullpath);
        forwardFile(eaddr, email);
    }

    remove(formattedFile.c_str());
}

string EmailForwarderFormatter::newBoundary()
{
    unsigned long rn = Formatter::random_number() * (double) ULONG_MAX;
    std::ostringstream out;
    out << "_RFE_";
    out << hex << rn;
    rn = random_number() * (double) ULONG_MAX;
    out << hex << rn;
    return (EmailExtractor::str_toupper(out.str()) + "reutersfastemail_");
}

// format the forwared email as an attachment
string EmailForwarderFormatter::formatFile(string to, string fullpath)
{
    string md;
    string boundary = newBoundary();
    EmailExtractor::ToSet dummyset;
    string email = "Date: " + date() + "\r\n";
    email += "From: " + m_from + "\r\n";
    email += "To: " + to + "\r\n";

    ifstream infile(fullpath, ifstream::in);
    string str;

    string  msgid;
    string  o_to;
    string  from;
    string  subject;
    string  date;
    string  contenttype;
    string  charset;
    string  transferenc;
    string  returnpath;
    string  envelope_from;
    EmailExtractor::ToSet original_to;
    EmailExtractor::ToSet delivered_to;
    EmailExtractor::ToSet envelope_to;

    EmailExtractor::scan_headers(infile,
        msgid,
        o_to,
        from,
        subject,
        date,
        contenttype,
        charset,
        transferenc,
        returnpath,
        envelope_from,
        original_to,
        delivered_to,
        envelope_to);

    if (MaildirFormatter::sha256_as_str((void *) msgid.c_str(), msgid.length(), md))
        email += "Message-ID: <" + md + "@fastemail.tr>" + "\r\n";
    else
        email += "Message-ID: <" + msgid + "@fastemail.tr>" + "\r\n";
    email += "References: <" + msgid + ">" + "\r\n";

    email += "user-agent: reuters-fast-email\r\n";
    email += "Content-Type: multipart/mixed;\r\n boundary=" + boundary + "\r\n";
    email += "Original-recipient: rfc822;" + o_to + "\r\n";
    email += "Subject: FW: " + subject + "\r\n";
    email += "\r\n";
    email += "--" + boundary + "\r\n";
    email += "Content-Disposition: attachment;\r\n filename=\"" + subject.substr(0, 75) + ".eml\"\r\n";
    email += "Content-Type: message/rfc822;\r\n name=\"" + subject.substr(0, 75) + ".eml\"\r\n";
    //email += "Content-Transfer-Encoding: base64\r\n";
    if (transferenc == "")
        email += "Content-Transfer-Encoding: 7bit\r\n";
    else
        email += "Content-Transfer-Encoding: " + transferenc + "\r\n";

    email += "\r\n";

    infile.seekg(0, std::ios::end);
    str.reserve(infile.tellg());
    infile.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    email += str + "\r\n";

    email += "--" + boundary + "--\r\n";

    infile.close();

    return email;
}

void EmailForwarderFormatter::forwardFile(string env_to, string email)
{
    if (m_verbose >= 4)
    {
        cout << "---------------------------------------------------------------------------------" << endl;
        cout << email << endl;
        cout << "---------------------------------------------------------------------------------" << endl;
    }

    if (m_verbose >= 3)
        cout << "Forwarding email to " << env_to << " using outgoing mail server " << m_server_info.server << ", username " << m_server_info.username << endl;

    SMTPSender sender(m_domain, m_server_info, m_verbose);
    sender.send(email, env_to, m_from);
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

