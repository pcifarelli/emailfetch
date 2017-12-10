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
#include <iconv.h>
#include <cstddef>
#include <string>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <fstream>
#include <ios>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "UCDPEmailExtractor.h"
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
    Aws::String most_of_the_date;
    string msgid;
    string date;

    UCDPEmailExtractor email(m_fullpath.c_str());
    // use rfc2822 msgid, if present.  It usually is, but it isn't strictly required
    if (!email.msgid().length())
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
