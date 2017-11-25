/*
 * UCDPDownloader.cpp
 *
 *  Created on: Oct 29, 2017
 *      Author: paulc
 */

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <stdio.h>
#include <cstddef>
#include <string>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <fstream>
#include <ios>
#include <sstream>
#include "UCDPFormatter.h"

using namespace std;

UCDPFormatter::UCDPFormatter(Aws::String dir) :
    Formatter(dir + "full/")
{

}

UCDPFormatter::~UCDPFormatter()
{
    // TODO Auto-generated destructor stub
}

void UCDPFormatter::open(const Aws::S3::Model::Object obj, const ios_base::openmode mode)
{
    m_objkey = obj.GetKey().c_str();
    m_message_drop_time = obj.GetLastModified();

    Formatter::open(obj, mode);
}

void UCDPFormatter::clean_up()
{
    string msgid;
    string to;
    string from;
    string subject;
    string date;
    string contenttype;
    string boundary;
    Aws::String most_of_the_date;

    scan_headers(m_fullpath.c_str(), msgid, to, from, subject, date, contenttype, boundary);
    // use rfc2822 msgid, if present.  It usually is, but it isn't strictly required
    if (!msgid.length())
        msgid = m_objkey;

    most_of_the_date = m_message_drop_time.ToGmtString("%Y-%m-%dT%H:%M:%S");
    int64_t secs_ms = m_message_drop_time.Millis(); // milliseconds since epoch
    chrono::system_clock::time_point time_se = m_message_drop_time.UnderlyingTimestamp();
    time_t t = std::chrono::system_clock::to_time_t(time_se);
    int64_t ms = secs_ms - (t * 1000);

    ostringstream o;
    o << most_of_the_date << '.' << setfill('0') << setw(3) << ms << "Z";
    date = o.str();

}

int UCDPFormatter::scan_headers(const string fname, string &msgid, string &to, string &from, string &subject, string &date,
    string &contenttype, string &boundary)
{
    ifstream infile(fname, ifstream::in);

    string prev = "", s = "", next = "";
    msgid = to = from = subject = date = contenttype = boundary = "";
    regex e_msgid("^(Message-ID:).*<(.*)>(.*)");
    regex e_to("^(To:)(.*)");
    regex e_from("^(From:)(.*)");
    regex e_subject("^(Subject:)(.*)");
    regex e_date("^(Date:)(.*)");
    regex e_contenttype("^(Content-Type:)(.*)");
    smatch sm;

    bool last = false;
    while ((infile.good() || last)
        && !(msgid.length() && to.length() && from.length() && subject.length() && date.length() && contenttype.length()))
    {
        getline(infile, next);

        // chomp off any cr left over from windows text
        auto c = next.cend();
        c--;
        if (*c == '\r')
            next.erase(c);

        regex_match(s, sm, e_msgid);
        if (!msgid.length() && sm.size() > 0)
            msgid = sm[2];

        regex_match(s, sm, e_to);
        if (!to.length() && sm.size() > 0)
            to = sm[2];

        regex_match(s, sm, e_from);
        if (!from.length() && sm.size() > 0)
            from = sm[2];

        regex_match(s, sm, e_subject);
        if (!subject.length() && sm.size() > 0)
            subject = sm[2];

        regex_match(s, sm, e_date);
        if (!date.length() && sm.size() > 0)
            date = sm[2];

        regex_match(s, sm, e_contenttype);
        if (!contenttype.length() && sm.size() > 0)
        {
            regex e_boundary1("^[ \t]*multipart/(.*)");
            regex e_boundary2("^[ \t]*multipart/([^ \t]+)boundary=\"([^\"]+)\"");
            regex e_boundary3("^[ \t]*boundary=\"([^\"]+)\"");
            smatch sm1;
            smatch sm2;
            smatch sm_next;
            string ct = sm[2].str();

            regex_match(ct, sm1, e_boundary1);
            regex_match(ct, sm2, e_boundary2);
            regex_match(next, sm_next, e_boundary3);
            if (sm1.size() > 0)
            {
                if (sm2.size())
                {
                    boundary = sm2[3];
                    contenttype = "multipart/" + sm2[2].str();
                }
                else if (sm_next.size())
                {
                    boundary = sm_next[1];
                    contenttype = sm[2];
                }
            }
            else
                contenttype = sm[2];

            // string the semi-colon
            auto c = contenttype.cend();
            c--;
            if (*c == ';')
                contenttype.erase(c);
        }
        prev = s;
        s = next;
        if (!infile.good())
        {
            if (last)
                last = false;
            else
                last = true;
        }
    }

    return 0;
}

string UCDPFormatter::escape_json(const string &s)
{
    ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++)
    {
        switch (*c)
        {
        case '"':
            o << "\\\"";
            break;
        case '\\':
            o << "\\\\";
            break;
        case '\b':
            o << "\\b";
            break;
        case '\f':
            o << "\\f";
            break;
        case '\n':
            o << "\\n";
            break;
        case '\r':
            o << "\\r";
            break;
        case '\t':
            o << "\\t";
            break;
        default:
            if ('\x00' <= *c && *c <= '\x1f')
            {
                o << "\\u" << hex << setw(4) << setfill('0') << (int) *c;
            }
            else
            {
                o << *c;
            }
        }
    }
    return o.str();
}

std::string UCDPFormatter::unescape_json(const string &s)
{
    ostringstream o;
    auto c = s.cbegin();
    while (c != s.cend())
    {
        if (*c == '\\')
        {
            auto p = c + 1;
            if (p != s.cend())
            {
                switch (*p)
                {
                case '"':
                    o << '"';
                    break;
                case '\\':
                    o << '\\';
                    break;
                case 'b':
                    o << '\b';
                    break;
                case 'f':
                    o << '\f';
                    break;
                case 'n':
                    o << '\n';
                    break;
                case 'r':
                    o << '\r';
                    break;
                case 't':
                    o << '\t';
                    break;
                case 'u':
                {
                    if ((p + 1) != s.cend() && (p + 2) != s.cend() && (p + 3) != s.cend() && (p + 4) != s.cend())
                    {
                        char *stop;
                        string hex = "0x";
                        p++;
                        hex += *p;
                        p++;
                        hex += *p;
                        p++;
                        hex += *p;
                        p++;
                        hex += *p;
                        o << (char) strtol(hex.c_str(), &stop, 16);
                    }
                    else
                    {
                        o << '\\';
                        while (p != s.cend())
                        {
                            o << *p;
                            p++;
                        }
                    }
                }
                    break;
                }
                c = p;
            }
            else
                o << '\\' << *c;
        }
        else
            o << *c;

        c++;
    }
    return o.str();
}
