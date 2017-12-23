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
#include <unistd.h>
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
    int         trmessageprio,
    bool validate_json,
    int verbose) :
    Formatter(dir + "full/"),
    m_ip(ip), m_certificate(certificate), m_certpassword(certpassword), m_snihostname(snihostname), m_port(port),
    m_trclientid(trclientid), m_trfeedid(trfeedid), m_trmessagetype(trmessagetype), m_trmessageprio(trmessageprio),
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
    // see if we failed to post the last email we were working on to completion, and finish it up before working on this one
    recover();

    m_objkey = obj.GetKey().c_str();
    m_message_drop_time = obj.GetLastModified();

    Formatter::open(obj, mode);
}

void UCDPFormatter::recover()
{
    // check for markers indicating incomplete process

    // process the old email

    // remove the markers
}

string UCDPFormatter::stash(string fname)
{
    string nfname = fname;
    std::size_t nn = nfname.find_last_of('/');
    if (nn != std::string::npos)
        nfname.insert(nn+1, 1, '.');
    else
        nfname.insert(0, 1, '.');

    rename(fname.c_str(), nfname.c_str());
    return nfname;
}

string UCDPFormatter::unstash(std::string fname)
{
    string nfname = fname;

    std::size_t nn = nfname.find_last_of('/');
    if (nn != std::string::npos)
        nfname.insert(nn+1, 1, '.');
    else
        nfname.insert(0, 1, '.');

    rename(nfname.c_str(), fname.c_str());
    return fname;
}


void UCDPFormatter::processEmail(std::string fname)
{
    Aws::String most_of_the_date;
    string msgid;
    string date;

    // Since the email extractor saves everything in memory, lets keep it on the heap
    EmailExtractor *email = new EmailExtractor(fname);

    // use rfc2822 msgid, if present.  It usually is, but it isn't strictly required
    if (!email->msgid().length())
        msgid = m_objkey;
    else
        msgid = email->msgid();

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


void UCDPFormatter::clean_up()
{
    // the idea is that the downloader will ignore files that start with a '.'
    // stash() renames the file to start with the '.' - this way if we are interrupted
    // while processing this file, the downloader will fetch it again when we restart.
    string fname = stash(m_fullpath.c_str());
    processEmail(fname);
    unstash(m_fullpath.c_str());
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

string UCDPFormatter::formatMessage1(ostringstream &o, string msgid, string date, EmailExtractor *email, bool validate)
{
    string trmsgid, post_trmsgid;

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
            cout << "Failed to validate json (Message 1): ";
            cout << "S3 objkey=" << m_objkey << " ";
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
        cout << "trmessageid: " << trmsgid << endl;
        cout << o.str() << endl;
    }

    return trmsgid;
}

string UCDPFormatter::formatMessage2(ostringstream &o, string msgid, string date, EmailExtractor *email, bool validate)
{
    string trmsgid, post_trmsgid;
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
            cout << "Failed to validate json (Message 2): ";
            cout << "S3 objkey=" << m_objkey << " ";
            cout << o.str() << endl;
        }
    }

    trmsgid = msgid + "_2";
    if (MaildirFormatter::sha256_as_str((void *) trmsgid.c_str(), trmsgid.length(), post_trmsgid))
        trmsgid = post_trmsgid;

    if (m_verbose >= 3)
    {
        cout << "Message 2: " << endl;
        cout << "trmessageid: " << trmsgid << endl;
        cout << o.str() << endl;
    }

    return trmsgid;
}

string UCDPFormatter::formatMessage3(ostringstream &o, string msgid, string date, EmailExtractor *email, bool validate)
{
    string trmsgid, post_trmsgid;

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
            cout << "Failed to validate json (Message 3): ";
            cout << "S3 objkey=" << m_objkey << " ";
            cout << o.str() << endl;
        }
    }

    trmsgid = msgid + "_3";
    if (MaildirFormatter::sha256_as_str((void *) trmsgid.c_str(), trmsgid.length(), post_trmsgid))
        trmsgid = post_trmsgid;

    if (m_verbose >= 3)
    {
        cout << "Message 3: " << endl;
        cout << "trmessageid: " << trmsgid << endl;
        cout << o.str() << endl;
    }

    return trmsgid;
}

string UCDPFormatter::formatMessage4(ostringstream &o, int num, string msgid, string date, EmailExtractor::Body &b, bool validate)
{
    string trmsgid, post_trmsgid;


    o << "{";
    o << "\"" << "messageId"   << "\":\"" << msgid            << "\"" << ",";
    o << "\"" << "dateTime"    << "\":\"" << date             << "\"" << ",";
    if (b.contenttype() != "" && b.charset() != "")
        o << "\"" << "contentType" << "\":\"" << b.contenttype() << "; " << "charset=" << b.charset() << "\"" << ",";
    else if (b.contenttype() != "")
        o << "\"" << "contentType" << "\":\"" << b.contenttype() << "; " << "charset=" << "us-ascii" << "\"" << ",";
    else
        o << "\"" << "contentType" << "\":\"" << "text/plain" << "; " << "charset=" << "us-ascii" << "\"" << ",";

    o << "\"" << "messageTransferEncoding" << "\":\"" << "base64" << "\"" << ",";
    o << "\"" << "body" << "\":\"" << b.asBase64() << "\"";
    o << "}";

    if (validate)
    {
        istringstream istrm( o.str() );
        Aws::Utils::Json::JsonValue jv(istrm);
        if (!jv.WasParseSuccessful())
        {
            cout << "Failed to validate json (Message 4, body " << num << ") ";
            cout << "S3 objkey=" << m_objkey << " ";
            cout << o.str() << endl;
        }
    }

    trmsgid = msgid + "_4_" + to_string(num);
    if (MaildirFormatter::sha256_as_str((void *) trmsgid.c_str(), trmsgid.length(), post_trmsgid))
        trmsgid = post_trmsgid;

    if (m_verbose >= 3)
    {
        cout << "Message 4, body " << num << endl;
        cout << "trmessageid: " << trmsgid << endl;
        if (m_verbose >= 4)
            cout << o.str() << endl;
    }

    return trmsgid;
}

string UCDPFormatter::formatMessage5(ostringstream &o, int num, string msgid, string date, EmailExtractor::Attachment &a, bool validate)
{
    string trmsgid, post_trmsgid;

    // the EmailExtractor catagorizes something as an attachment if it has a filename - with out one, it adds it to the list of bodies
    // so we can assume that filename() always returns something
    o << "{";
    o << "\"" << "messageId" << "\":\"" << msgid            << "\"" << ",";
    o << "\"" << "dateTime"  << "\":\"" << date             << "\"" << ",";
    if (a.contenttype() != "")
        o << "\"" << "contentType" << "\":\"" << a.contenttype() << "; " << "name=\\\"" << a.filename() << "\\\"\"" << ",";
    else
        o << "\"" << "contentType" << "\":\"" << "text/plain" << "; " << "name=\\\"" << a.filename() << "\\\"\"" << ",";

    o << "\"" << "messageTransferEncoding" << "\":\"" << "base64" << "\"" << ",";
    o << "\"" << "attachmentName" << "\":\"" << a.filename() << "\"" << ",";
    o << "\"" << "attachmentData" << "\":\"" << a.asBase64() << "\"";
    o << "}";

    if (validate)
    {
        istringstream istrm( o.str() );
        Aws::Utils::Json::JsonValue jv(istrm);
        if (!jv.WasParseSuccessful())
        {
            cout << "Failed to validate json (Message 5, attachment " << num << ") ";
            cout << "S3 objkey=" << m_objkey << " ";
            cout << o.str() << endl;
        }
    }

    trmsgid = msgid + "_5_" + to_string(num);
    if (MaildirFormatter::sha256_as_str((void *) trmsgid.c_str(), trmsgid.length(), post_trmsgid))
        trmsgid = post_trmsgid;

    if (m_verbose >= 3)
    {
        cout << "Message 5, attachment " << num << endl;
        cout << "trmessageid: " << trmsgid << endl;
        if (m_verbose >= 4)
            cout << o.str() << endl;
    }

    return trmsgid;
}

void UCDPFormatter::check_result(string result, string trmsgid)
{
    istringstream istrm( result );
    Aws::Utils::Json::JsonValue jv(istrm);
    if (jv.ValueExists("tr-message-id"))
    {
        string res_trmsgid = jv.GetString("tr-message-id").c_str();
        if ( res_trmsgid != trmsgid)
        {
            cout << "ERROR: Unexpected result returned from post to UCDP (\"tr-message-id\" returned does not agree with posted tr-message-id) ";
            cout << "S3 objkey=" << m_objkey << endl;
        }
    }
    else if (!jv.ValueExists("tr-message-id"))
    {
        cout << "ERROR: Unexpected result returned from post to UCDP (\"tr-message-id\" missing) ";
        cout << "S3 objkey=" << m_objkey << endl;
    }

    if (jv.ValueExists("status"))
    {
        string res_status = jv.GetString("status").c_str();
        if ( res_status != "success" )
        {
            cout << "ERROR: Unexpected result returned from post to UCDP (\"status\" returned is not \"success\" ";
            cout << "S3 objkey=" << m_objkey << endl;
        }
    }
    else
    {
        cout << "ERROR: Unexpected result returned from post to UCDP (\"status\" missing) ";
        cout << "S3 objkey=" << m_objkey << endl;
    }
}

void UCDPFormatter::postToUCDP(string msgid, string date, EmailExtractor *email, bool validate)
{
    string trmsgid;
    string result;
    ostringstream o;
    UCDPCurlPoster poster(m_ip, m_certificate, m_certpassword, m_snihostname, m_port, true, m_trclientid, m_trfeedid, m_trmessagetype);

    trmsgid = formatMessage1(o, msgid, date, email, validate);
    poster.post( m_trmessageprio, trmsgid, o.str() );
    result = poster.getResult();
    if (m_verbose >=3 )
        cout << result << endl;
    check_result(result, trmsgid);

    o.str("");
    o.clear();

    trmsgid = formatMessage2(o, msgid, date, email, validate);
    poster.post( m_trmessageprio, trmsgid, o.str() );
    result = poster.getResult();
    if (m_verbose >=3 )
        cout << result << endl;;
    check_result(result, trmsgid);

    o.str("");
    o.clear();

    trmsgid = formatMessage3(o, msgid, date, email, validate);
    poster.post( m_trmessageprio, trmsgid, o.str() );
    result = poster.getResult();
    if (m_verbose >=3 )
        cout << result << endl;;
    check_result(result, trmsgid);

    o.str("");
    o.clear();

    // Message 4 (the bodies)
    int num = 0;
    for (auto &b : email->bodies())
    {
        num++;
        trmsgid = formatMessage4(o, num, msgid, date, b, validate);
        poster.post( m_trmessageprio, trmsgid, o.str() );
        result = poster.getResult();
        if (m_verbose >=3 )
            cout << result << endl;;
        check_result(result, trmsgid);

        o.str("");
        o.clear();
    }

    // Message 5 (the attachments)
    num = 0;
    for (auto &a : email->attachments())
    {
        num++;
        trmsgid = formatMessage5(o, num, msgid, date, a, validate);
        poster.post( m_trmessageprio, trmsgid, o.str() );
        result = poster.getResult();
        if (m_verbose >=3 )
            cout << result << endl;;
        check_result(result, trmsgid);

        o.str("");
        o.clear();
    }
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
