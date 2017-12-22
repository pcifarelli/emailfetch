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
#include <aws/core/utils/json/JsonSerializer.h>

#include "EmailExtractor.h"
#include "UCDPFormatter.h"

// for the SHA functions
#include "MaildirFormatter.h"

using namespace std;

UCDPFormatter::UCDPFormatter(
    Aws::String dir,
    std::string ip,
    std::string snihostname,
    unsigned short port,
    std::string certificate,
    std::string certpassword,
    std::string trclientid,
    std::string trfeedid,
    std::string trmessagetype,
    std::string trmessageprio,
    bool validate_json,
    int verbose) :
    Formatter(dir + "full/"),
    m_poster(ip, certificate, certpassword, snihostname, port, true, trclientid, trfeedid, trmessagetype),
    m_trmessageprio(trmessageprio),
    m_validate_json(validate_json),
    m_verbose(verbose)
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

    // Since the email extractor saves everything in memory, lets keep it on the heap
    EmailExtractor *email = new EmailExtractor(m_fullpath.c_str());

    // use rfc2822 msgid, if present.  It usually is, but it isn't strictly required
    if (!email->msgid().length())
        msgid = m_objkey;
    else
        msgid = email->msgid() + "^" + m_objkey;

    most_of_the_date = m_message_drop_time.ToGmtString("%Y-%m-%dT%H:%M:%S");
    int64_t secs_ms = m_message_drop_time.Millis(); // milliseconds since epoch
    chrono::system_clock::time_point time_se = m_message_drop_time.UnderlyingTimestamp();
    time_t t = std::chrono::system_clock::to_time_t(time_se);
    int64_t ms = secs_ms - (t * 1000);

    ostringstream o;
    o << most_of_the_date << '.' << setfill('0') << setw(3) << ms << "Z";
    date = o.str();

    postToUCDP(msgid, date, email, m_validate_json);

    delete email;
}

string UCDPFormatter::fmtToField( string csstr )
{
    std::istringstream iss(csstr);
    std::string innerarr;
    int num_tos = 0;

    // tokenize by comma
    std::string s;
    while (std::getline(iss, s, ','))
    {
        EmailExtractor::trim(s);
        if (s != "")
        {
            innerarr += "\"" + s + "\",";
            num_tos++;
        }
    }

    // remove the last comma
    auto c = innerarr.cend();
    c--;
    if (*c == ',')
        innerarr.erase(c);

    if (num_tos > 1)
        return "[" + innerarr + "]";

    return innerarr;
}

void UCDPFormatter::postToUCDP(string msgid, string date, EmailExtractor *email, bool validate)
{
    string trmsgid, post_trmsgid;
    ostringstream o;

    // Message 1
    o << "{";
    o << "\"" << "messageId" << "\":\"" << msgid         << "\"" << ",";
    o << "\"" << "dateTime"  << "\":\"" << date          << "\"" << ",";
    o << "\"" << "from"      << "\":\"" << email->from() << "\"";
    o << "}";

    if (validate)
    {
        istringstream istrm( o.str() );
        Aws::Utils::Json::JsonValue jv(istrm);
        if (!jv.WasParseSuccessful())
        {
            cout << "Failed to validate json (Message 1): " << endl;
            cout << o.str() << endl;
        }
    }

    // append a "1" and sha to get the trmsgid.  If it fails, just use the pre-sha trmsgid
    trmsgid = msgid + "_1";
    if (MaildirFormatter::sha256_as_str((void *) trmsgid.c_str(), trmsgid.length(), post_trmsgid))
        trmsgid = post_trmsgid;

    if (m_verbose >= 3)
    {
        cout << "Message 1: " << endl;
        cout << trmsgid << endl;
        cout << o.str() << endl;
    }

    // Now post
    // m_poster.post( m_trmessageprio, trmsgid, o.str() );

    o.str("");
    o.clear();

    // Message 2
    o << "{";
    o << "\"" << "messageId" << "\":\"" << msgid         << "\"" << ",";
    o << "\"" << "dateTime"  << "\":\"" << date          << "\"" << ",";
    o << "\"" << "to"        << "\":" << fmtToField(email->to());
    o << "}";

    if (validate)
    {
        istringstream istrm( o.str() );
        Aws::Utils::Json::JsonValue jv(istrm);
        if (!jv.WasParseSuccessful())
        {
            cout << "Failed to validate json (Message 2): " << endl;
            cout << o.str() << endl;
        }
    }

    trmsgid = msgid + "_2";
    if (MaildirFormatter::sha256_as_str((void *) trmsgid.c_str(), trmsgid.length(), post_trmsgid))
        trmsgid = post_trmsgid;

    if (m_verbose >= 3)
    {
        cout << "Message 2: " << endl;
        cout << trmsgid << endl;
        cout << o.str() << endl;
    }

    // Now post
    // m_poster.post( m_trmessageprio, trmsgid, o.str() );

    o.str("");
    o.clear();

    // Message 3
    o << "{";
    o << "\"" << "messageId" << "\":\"" << msgid            << "\"" << ",";
    o << "\"" << "dateTime"  << "\":\"" << date             << "\"" << ",";
    o << "\"" << "subject"   << "\":\"" << email->subject() << "\"";
    o << "}";

    if (validate)
    {
        istringstream istrm( o.str() );
        Aws::Utils::Json::JsonValue jv(istrm);
        if (!jv.WasParseSuccessful())
        {
            cout << "Failed to validate json (Message 3): " << endl;
            cout << o.str() << endl;
        }
    }

    trmsgid = msgid + "_3";
    if (MaildirFormatter::sha256_as_str((void *) trmsgid.c_str(), trmsgid.length(), post_trmsgid))
        trmsgid = post_trmsgid;

    if (m_verbose >= 3)
    {
        cout << "Message 3: " << endl;
        cout << trmsgid << endl;
        cout << o.str() << endl;
    }

    // Now post
    // m_poster.post( m_trmessageprio, trmsgid, o.str() );

    o.str("");
    o.clear();

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
