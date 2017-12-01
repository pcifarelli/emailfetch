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

#define UTF8_MAX 6
int UCDPFormatter::to_utf8(string charset, vector<unsigned char> &in, vector<unsigned char> &out)
{
    string chenc = str_toupper(charset);
    size_t iconv_bytes_in = in.size(), iconv_bytes_out = in.size() * UTF8_MAX;

    iconv_t cd = iconv_open("UTF-8", chenc.c_str());
    if (cd == (iconv_t) -1)
    {
	cout << "ERROR opening iconv" << endl;
	return -1;
    }

    char *pin = (char *) &in[0];
    char *pout = (char *) malloc( UTF8_MAX * in.size() );
    char *pout_iconv = pout;
    int ret = iconv(cd, &pin, &iconv_bytes_in, &pout_iconv, &iconv_bytes_out);
    if (ret == (size_t) -1)
    {
	cout << "Could not convert to " << charset << " to UTF-8" << endl;
	return -1;
    }
    iconv_close(cd);
    size_t cnvtd = (size_t) (pout_iconv - pout);

    for (int i; i < cnvtd; i++)
 	out.insert(out.end(), pout[i]);

    free(pout);

    return 0;
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

string UCDPFormatter::strip_quotes(string &s)
{
    // strip the quotes if there
    auto c = s.cend();
    c--;
    if (*c == '\"')
	s.erase(c);

    c = s.cbegin();
    if (*c == '\"')
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

void UCDPFormatter::base64_encode(vector<unsigned char> &input, vector<unsigned char> &output, bool preserve_crlf)
{
  BIO *bmem, *b64;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, &input[0], input.size());
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  for (int i; i < bptr->length; i++)
      if (preserve_crlf || (bptr->data[i] != '\r' && bptr->data[i] != '\n'))
	  output.insert(output.end(), bptr->data[i]);

  BIO_free_all(b64);

}

void UCDPFormatter::base64_decode(vector<unsigned char> &input, vector<unsigned char> &output)
{
  BIO *b64, *bmem;
  int length = input.size();

  output.insert(output.end(), length, (char) 0);

  b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bmem = BIO_new_mem_buf(&input[0], length);
  bmem = BIO_push(b64, bmem);

  int i = 0;
  if ( (i = BIO_read(bmem, &output[0], length)) <= 0)
      cout << "base64 decode failed\n";
  else
      output.resize(i);

  BIO_free_all(bmem);
}

string UCDPFormatter::str_tolower(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return tolower(c); });
    return s;
}

string UCDPFormatter::str_toupper(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return toupper(c); });
    return s;
}

bool UCDPFormatter::is_utf8(string charset)
{
    charset = str_tolower(charset);
    return ( (charset == "utf8" || charset == "utf-8") );
}

void UCDPFormatter::read_ifstream1(ifstream &infile, vector<unsigned char> &rawbody)
{
    streamsize n2;
    vector<unsigned char> buf2(1024);
    do
    {
	if (buf2.size() < 1024)
	    buf2.reserve(1024);
        n2 = infile.readsome((char *) &buf2[0], 1024);
	if (n2 < 1024)
	    buf2.resize(n2);
        rawbody.insert(rawbody.end(), buf2.begin(), buf2.end());
    } while (infile.good() && n2);

}

void UCDPFormatter::read_ifstream(ifstream &infile, vector<unsigned char> &rawbody)
{
    string next;
    while (infile.good())
    {
       getline(infile, next);
       // chomp off any cr left over from windows text
       auto c = next.cend();
       c--;
       if (*c == '\r')
          next.erase(c);

       if (!next.length())
	   break;

       rawbody.insert(rawbody.end(), next.cbegin(), next.cend());
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
