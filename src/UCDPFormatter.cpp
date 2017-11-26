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
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

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
    string charset;
    string transferenc;
    Aws::String most_of_the_date;

    scan_headers(m_fullpath.c_str(), msgid, to, from, subject, date, contenttype, boundary, charset, transferenc);
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

// trim from start (in place)
void UCDPFormatter::ltrim(string &s)
{
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch)
    {
        return !isspace(ch);
    }));
}

// trim from end (in place)
void UCDPFormatter::rtrim(string &s)
{
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch)
    {
        return !isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void UCDPFormatter::trim(string &s)
{
    ltrim(s);
    rtrim(s);
}

string UCDPFormatter::strip_semi(string &s)
{
    // strip the semi-colon if its there
    auto c = s.cend();
    c--;
    if (*c == ';')
        s.erase(c);

    return s;
}

string UCDPFormatter::extract_attr(string attr, string line)
{
    string value = "";

    int pos_cs = line.find(attr);
    if (pos_cs != string::npos)
    {
        string subline;
        subline = line.substr(pos_cs + attr.length());

        regex e("[; \t]");
        smatch sm;
        regex_search(subline, sm, e);
        value = subline.substr(1, sm.position(0));
        strip_semi(value);
    }

    return value;
}

void UCDPFormatter::extract_contenttype(string s, string next, string &contenttype, string &boundary, string &charset)
{
    regex e_contenttype("^(Content-Type:)(.*)");
    regex e_boundary1("^([ \t]*multipart/)(.*)");
    regex e_boundary2("^([a-zA-Z]+); (.*)");
    regex e_boundary3("^([ \t]*)(boundary=)\"(.*)\";?(.*)");
    smatch sm;
    smatch sm1;
    smatch sm2;
    smatch sm_next;

    regex_match(s, sm, e_contenttype);
    if (!contenttype.length() && sm.size() > 0)
    {
        string ct = sm[2].str();

        regex_match(ct, sm1, e_boundary1);
        regex_match(next, sm_next, e_boundary3);
        if (sm1.size() > 0)
        {
            string mpt = sm1[2].str();
            regex_match(mpt, sm2, e_boundary2);
            if (sm2.size())
            {
                smatch smb;
                string b = sm2[2].str();
                regex_match(b, smb, e_boundary3);
                boundary = smb[3];
                contenttype = "multipart/" + sm2[1].str();
                charset = extract_attr("charset", b);
            }
            else if (sm_next.size())
            {
                boundary = sm_next[3];
                contenttype = sm[2];
            }
        }
        else
            contenttype = sm[2];

        if (!charset.length())
            charset = extract_attr("charset", contenttype);

        trim(contenttype);

        regex e("[; \t]");
        smatch sm;
        regex_search(contenttype, sm, e);
        if (sm.size())
            contenttype = contenttype.substr(0, sm.position(0));

        strip_semi(contenttype);
    }
}

int UCDPFormatter::scan_attachment_headers(ifstream &f, string &contenttype, string &boundary, string &charset, string &transferenc,
    string &attachment_filename)
{
}

int UCDPFormatter::scan_headers(ifstream &infile, string &msgid, string &to, string &from, string &subject, string &date,
    string &contenttype, string &boundary, string &charset, string &transferenc)
{
    string prev = "", s = "", next = "";
    regex e_msgid("^(Message-ID:).*<(.*)>(.*)");
    regex e_to("^(To:)[ \t]*<?(.*)>?");
    regex e_from("^(From:)[ \t]*<?(.*)>?");
    regex e_subject("^(Subject:)(.*)");
    regex e_date("^(Date:)(.*)");
    regex e_trenc("^(Content-Transfer-Encoding:)(.*)");
    smatch sm;

    bool last = false;
    while ((infile.good() || last)
        && !(msgid.length() && to.length() && from.length() && subject.length() && date.length() && contenttype.length() && transferenc.length()))
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

        regex_match(s, sm, e_trenc);
        if (!transferenc.length() && sm.size() > 0)
        {
            transferenc = sm[2];
            trim(transferenc);
        }

        extract_contenttype(s, next, contenttype, boundary, charset);

        prev = s;
        s = next;
        if (!infile.good())
        {
            if (last)
                last = false;
            else
                last = true;
        }

        if (!s.length()) // end of header section
            break;
    }

}

int UCDPFormatter::scan_headers(const string fname, string &msgid, string &to, string &from, string &subject, string &date,
    string &contenttype, string &boundary, string &charset, string &transferenc)
{
    ifstream infile(fname, ifstream::in);

    msgid = to = from = subject = date = contenttype = boundary = charset = transferenc = "";

    scan_headers(infile, msgid, to, from, subject, date, contenttype, boundary, charset, transferenc);

    infile.close();
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
