/*
 * EmailExtractor.cpp
 *
 *  Created on: Dec 10, 2017
 *      Author: paulc
 */

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

#include "EmailExtractor.h"

using namespace std;

// accessors
const string &EmailExtractor::Body::body()
{
    m_str_error = "";
    m_error = EmailExtractor::NO_ERROR;
    return m_body;
}
const string &EmailExtractor::Body::contenttype()
{
    m_str_error = "";
    m_error = EmailExtractor::NO_ERROR;
    return m_contenttype;
}
const string &EmailExtractor::Body::charset()
{
    m_str_error = "";
    m_error = EmailExtractor::NO_ERROR;
    return m_charset;
}
const string &EmailExtractor::Body::transferenc()
{
    m_str_error = "";
    m_error = EmailExtractor::NO_ERROR;
    return m_transferenc;
}
const string &EmailExtractor::Body::contentID()
{
    m_str_error = "";
    m_error = EmailExtractor::NO_ERROR;
    return m_contentid;
}

string EmailExtractor::Body::asBase64()
{
    m_str_error = "";
    m_error = EmailExtractor::NO_ERROR;
    string body;
    if (m_transferenc != "base64")
    {
        string qpdbody;
        if (m_transferenc == "quoted-printable")
        {
            if (EmailExtractor::quoted_printable_decode(m_body, qpdbody))
            {
                m_str_error = "Unable to decode from quoted-printable";
                m_error = EmailExtractor::ERROR_QPDECODE;
            }

            if (EmailExtractor::base64_encode(qpdbody, body))
            {
                m_str_error = "Unable to encode as base64";
                m_error = EmailExtractor::ERROR_B64ENCODE;
            }
        }
        else
        {
            if (EmailExtractor::base64_encode(m_body, body))
            {
                m_str_error = "Unable to encode as base64";
                m_error = EmailExtractor::ERROR_B64ENCODE;
            }
        }
        return body;
    }

    return m_body;
}

string EmailExtractor::Body::asUtf8()
{
    string body;
    vector<unsigned char> uvbody;

    m_str_error = "";
    m_error = EmailExtractor::NO_ERROR;
    body.clear();
    // convert everything to utf-8
    if (m_transferenc == "base64" && m_charset != "" && !EmailExtractor::is_utf8(m_charset))
    {
        vector<unsigned char> vbody;
        if (EmailExtractor::base64_decode(m_body, vbody))
        {
            m_str_error = "Unable to decode from base64";
            m_error = EmailExtractor::ERROR_B64DECODE;
        }
        if (to_utf8(m_charset, vbody, uvbody))
        {
            m_str_error = "Unable to convert to utf8";
            m_error = EmailExtractor::ERROR_UTF8CONV;
        }
        body.insert(body.end(), uvbody.cbegin(), uvbody.cend());
        return body;
    }
    else if (m_transferenc == "quoted-printable" && m_charset != "" && !EmailExtractor::is_utf8(m_charset))
    {
        vector<unsigned char> vbody;
        if (EmailExtractor::quoted_printable_decode(m_body, body))
        {
            m_str_error = "Unable to decode from quoted-printable";
            m_error = EmailExtractor::ERROR_QPDECODE;
        }
        vbody.insert(vbody.end(), body.cbegin(), body.cend());
        body.clear();
        if (to_utf8(m_charset, vbody, uvbody))
        {
            m_str_error = "Unable to convert to utf8";
            m_error = EmailExtractor::ERROR_UTF8CONV;
        }
        body.insert(body.end(), uvbody.cbegin(), uvbody.cend());
        return body;
    }
    else if (m_charset != "" && !EmailExtractor::is_utf8(m_charset))
    {
        vector<unsigned char> vbody(m_body.cbegin(), m_body.cend());
        if (to_utf8(m_charset, vbody, uvbody))
        {
            m_str_error = "Unable to convert to utf8";
            m_error = EmailExtractor::ERROR_UTF8CONV;
        }
        body.insert(body.end(), uvbody.cbegin(), uvbody.cend());
        return body;
    }
    else if (m_transferenc == "base64")
    {
        if (EmailExtractor::base64_decode(m_body, body))
        {
            m_str_error = "Unable to decode from base64";
            m_error = EmailExtractor::ERROR_B64DECODE;
        }
        return body;
    }
    else if (m_transferenc == "quoted-printable")
    {
        if (EmailExtractor::quoted_printable_decode(m_body, body))
        {
            m_str_error = "Unable to decode from quoted-printable";
            m_error = EmailExtractor::ERROR_QPDECODE;
        }
        return body;
    }

    return m_body;
}

string EmailExtractor::Body::asUtf8Base64()
{
    m_str_error = "";
    m_error = EmailExtractor::NO_ERROR;
    string ubody = asUtf8();
    string body;

    if (!m_error)
        if (EmailExtractor::base64_encode(m_body, body))
        {
            m_str_error = "Unable to encode as base64";
            m_error = EmailExtractor::ERROR_B64ENCODE;
        }

    return body;
}

int EmailExtractor::Body::error() const
{
    return m_error;
}

string EmailExtractor::Body::str_error() const
{
    return m_str_error;
}

const string &EmailExtractor::Attachment::attachment()
{
    return m_attachment;
}
const string &EmailExtractor::Attachment::filename()
{
    return m_attachment_filename;
}
const string &EmailExtractor::Attachment::creation_time()
{
    return m_creation_datetime;
}
const string &EmailExtractor::Attachment::modification_time()
{
    return m_creation_datetime;
}
const string &EmailExtractor::Attachment::contenttype()
{
    return m_contenttype;
}
const string &EmailExtractor::Attachment::charset()
{
    return m_charset;
}
const string &EmailExtractor::Attachment::transferenc()
{
    return m_transferenc;
}
const int EmailExtractor::Attachment::size()
{
    return m_size;
}

string EmailExtractor::Attachment::asBase64()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    if (m_transferenc != "base64")
    {
        string attachment;
        if (EmailExtractor::base64_encode(m_attachment, attachment))
        {
            m_error = EmailExtractor::ERROR_B64ENCODE;
            m_str_error = "Unable to encode as base64";
        }
        return attachment;
    }

    return m_attachment;
}

// Save the attachment in the specified directory
void EmailExtractor::Attachment::save_attachment(string dirname)
{
    vector<unsigned char> attachment;
    string c;

    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    std::size_t nn = dirname.find_last_of('/');
    if (nn != string::npos && nn != dirname.length() - 1)
        c = "/";
    dirname += c;

    if (m_transferenc == "base64")
    {
        if (EmailExtractor::base64_decode(m_attachment, attachment))
        {
            m_error = EmailExtractor::ERROR_B64DECODE;
            m_str_error = "Unable to decode base64 for attachment " + m_attachment_filename;
            return;
        }
        ofstream o(dirname + m_attachment_filename);
        if (!o.is_open() || !o.good())
        {
            m_error = EmailExtractor::ERROR_NOOPEN;
            m_str_error = "Unable to open file for writing attachment " + m_attachment_filename;
        }
        o.write((const char *) &attachment[0], attachment.size());
        o.close();
    }
    else
    {
        ofstream o(dirname + m_attachment_filename);
        if (!o.is_open() || !o.good())
        {
            m_error = EmailExtractor::ERROR_NOOPEN;
            m_str_error = "Unable to open file for writing attachment " + m_attachment_filename;
        }
        o.write((const char *) m_attachment.c_str(), m_attachment.length());
        o.close();
    }
}

int EmailExtractor::Attachment::error() const
{
    return m_error;
}

string EmailExtractor::Attachment::str_error() const
{
    return m_str_error;
}

const string &EmailExtractor::msgid()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_msgid;
}
const string &EmailExtractor::to()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_to;
}
const string &EmailExtractor::from()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_from;
}
const string &EmailExtractor::subject()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_subject;
}
const string &EmailExtractor::date()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_date;
}
const string &EmailExtractor::contenttype()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_contenttype;
}
const string &EmailExtractor::charset()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_charset;
}
const string &EmailExtractor::transferenc()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_transferenc;
}

int EmailExtractor::num_bodies()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_bodies.size();
}
int EmailExtractor::num_attachments()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_attachments.size();
}
EmailExtractor::BodyList &EmailExtractor::bodies()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_bodies;
}
EmailExtractor::AttachmentList &EmailExtractor::attachments()
{
    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    return m_attachments;
}

EmailExtractor::EmailExtractor(const string fullpath) :
    m_fullpath(fullpath)
{
    m_boundaries.clear();

    m_error = EmailExtractor::NO_ERROR;
    m_str_error = "";

    save_parts(m_fullpath.c_str());
}

EmailExtractor::~EmailExtractor()
{
    m_bodies.clear();
    m_attachments.clear();
    m_boundaries.clear();
}

int EmailExtractor::error() const
{
    return m_error;
}

string EmailExtractor::str_error() const
{
    return m_str_error;
}

int EmailExtractor::to_utf8(string charset, vector<unsigned char> &in, vector<unsigned char> &out)
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
    char *pout = (char *) malloc(UTF8_MAX * in.size());
    char *pout_iconv = pout;
    int ret = iconv(cd, &pin, &iconv_bytes_in, &pout_iconv, &iconv_bytes_out);
    if (ret == (size_t) -1)
    {
        cout << "Could not convert " << charset << " to UTF-8" << endl;
        free(pout);
        return -1;
    }
    iconv_close(cd);
    size_t cnvtd = (size_t) (pout_iconv - pout);

    for (int i = 0; i < cnvtd; i++)
        out.insert(out.end(), pout[i]);

    free(pout);

    return 0;
}

bool EmailExtractor::extract_body(istream &infile, string contenttype, string transferenc, string &charset, string &body)
{
    bool is_text = !contenttype.compare(0, 4, "text");
    bool strip_crlf = (str_tolower(transferenc) == "base64");
    vector<unsigned char> rawbody;
    bool reached_prev_boundary;

    rawbody.clear();
    reached_prev_boundary = read_istream_to_boundary(infile, m_boundaries, rawbody, strip_crlf);

    body.clear();
    body.insert(body.end(), rawbody.cbegin(), rawbody.cend());

    return reached_prev_boundary;
}

int EmailExtractor::scan_attachment_headers(istream &infile,
    string &contenttype,
    string &charset,
    string &transferenc,
    string &contentid,
    string &contentdisposition,
    BoundaryList &boundaries,
    Attachment &att)
{
    string dummy;
    string a_contenttype, a_charset, a_transferenc, a_contentid, a_contentdisposition;
    Attachment a_att;
    contenttype.clear();
    charset.clear();
    transferenc.clear();
    contentdisposition.clear();
    att.m_attachment_filename.clear();
    att.m_creation_datetime.clear();
    att.m_modification_datetime.clear();
    att.m_size = 0;
    int ret = scan_headers(infile, dummy, dummy, dummy, dummy, dummy, a_contenttype, a_charset, a_transferenc, a_contentid, a_contentdisposition, boundaries, a_att);
    if (a_contenttype.length())
        contenttype = a_contenttype;
    if (a_charset.length())
        charset = a_charset;
    if (a_transferenc.length())
        transferenc = a_transferenc;
    if (a_contentid.length())
        contentid = a_contentid;
    if (a_contentdisposition.length())
    {
        contentdisposition = a_contentdisposition;
        if (str_tolower(contentdisposition) == "attachment" || str_tolower(contentdisposition) == "inline")
            att = a_att;
    }

    return 0;
}

bool EmailExtractor::extract_all(istream &infile, string contenttype, string charset, string transferenc)
{
    string a_contenttype = contenttype, a_charset = charset, a_transferenc = transferenc;
    string contentid = "";
    bool reached_prev_boundary = false;
    string contentdisposition = "";
    Attachment att;

    while (infile.good() || reached_prev_boundary)
    {
        if ((str_tolower(contentdisposition) == "attachment" || str_tolower(contentdisposition) == "inline")
            && att.m_attachment_filename.length())
        {
            reached_prev_boundary = extract_body(infile, a_contenttype, a_transferenc, a_charset, att.m_attachment);
            att.m_contenttype = a_contenttype;
            att.m_charset = str_tolower(a_charset);
            att.m_transferenc = str_tolower(a_transferenc);

            m_attachments.push_back(att);
        }
        else if (a_contenttype == "message/rfc822" ||
                 a_contenttype == "message/rfc2046")    // would anyone use this as a contenttype? rfc822 is really about simple mail
        {
            string msgid, to, from, subject, date;
            string contenttype = "";
            string charset = a_charset;
            string transferenc = "";
            string contentid = "";
            string contentdisposition = "";
            Attachment att;

            scan_headers(infile,
                msgid,
                to,
                from,
                subject,
                date,
                contenttype,
                charset,
                transferenc,
                contentid,
                contentdisposition,
                m_boundaries,
                att);

            // extracting an attached email
            reached_prev_boundary = extract_all(infile, contenttype, charset, transferenc);
        }
        else
        {
            Body b;
            reached_prev_boundary = extract_body(infile, a_contenttype, a_transferenc, a_charset, b.m_body);

            // if it has some size, it's not just a newline, and it's content type is not multipart, then we save it
            if (b.m_body.size() && b.m_body != "\r\n" && a_contenttype.compare(0, 9, "multipart"))
            {
                // also, make sure there's something interesting

                regex e_boring("^([ \t\r\n]*)$");
                smatch sm;

                regex_match(b.m_body, sm, e_boring);
                if (!sm.size() &&
                    b.m_body.compare(0, 44, "This is a multi-part message in MIME format.")
                        )
                {
                    b.m_contenttype = a_contenttype;
                    b.m_charset = str_tolower(a_charset);
                    b.m_transferenc = str_tolower(a_transferenc);
                    b.m_contentid = contentid;

                    m_bodies.push_back(b);
                }
            }
        }

        if (reached_prev_boundary)
        {
            // we were extracting a subdocument
            return reached_prev_boundary;
        }

        string boundary = "";
        if (m_boundaries.size())
            boundary = m_boundaries.back();

        // see if there are new headers to deal with
        scan_attachment_headers(infile, a_contenttype, a_charset, a_transferenc, contentid, contentdisposition, m_boundaries, att);

        if (m_boundaries.size() && (m_boundaries.back() != boundary))
        {
            extract_all(infile, a_contenttype, a_charset, a_transferenc);

            // we returned because we reached the prev boundary, so we are possibly pointing to attachment headers, so check
            scan_attachment_headers(infile, a_contenttype, a_charset, a_transferenc, contentid, contentdisposition, m_boundaries, att);
        }
    }

    return reached_prev_boundary;
}

int EmailExtractor::save_parts(const string fname)
{
    ifstream infile(fname, ifstream::in);

    if (!infile.is_open() || !infile.good())
    {
        m_error = ERROR_NOOPEN;
        m_str_error = "Unable to open file";
        return -1;
    }

    save_parts(infile);
    infile.close();

    return 0;
}

int EmailExtractor::save_parts(string &buffer)
{
    stringstream in(buffer);

    save_parts(in);

    return 0;
}

int EmailExtractor::save_parts(std::istream &infile)
{
    string msgid, to, from, subject, date, contenttype, charset, transferenc, contentid, contentdisposition;
    Attachment att;

    // scan_headers leaves the ifstream pointing to the first line after the header block
    scan_headers(infile,
        m_msgid,
        m_to,
        m_from,
        m_subject,
        m_date,
        m_contenttype,
        m_charset,
        m_transferenc,
        contentid,
        contentdisposition,
        m_boundaries,
        att);

    extract_all(infile, m_contenttype, m_charset, m_transferenc);
    if (m_bodies.size() == 1)
    {
        if (m_bodies.front().m_contenttype == "")
            m_bodies.front().m_contenttype = m_contenttype;
        if (m_bodies.front().m_charset == "")
            m_bodies.front().m_charset = m_charset;
        if (m_bodies.front().m_transferenc == "")
            m_bodies.front().m_transferenc = m_transferenc;
        if (m_bodies.front().m_contentid == "")
            m_bodies.front().m_contentid = contentid;
    }

    return 0;
}

// trim from start (in place)
void EmailExtractor::ltrim(string &s)
{
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch)
    {
        return !isspace(ch);
    }));
}

// trim from end (in place)
void EmailExtractor::rtrim(string &s)
{
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch)
    {
        return !isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void EmailExtractor::trim(string &s)
{
    ltrim(s);
    rtrim(s);
}

string EmailExtractor::strip_semi(string &s)
{
    // strip the semi-colon if its there
    auto c = s.cend();
    c--;
    if (*c == ';')
        s.erase(c);

    return s;
}

string EmailExtractor::strip_quotes(string &s)
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

string EmailExtractor::extract_attr(string attr, string line)
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
        strip_quotes(value);
    }

    return value;
}

void EmailExtractor::extract_contenttype(vector<string> &lines, string &contenttype, BoundaryList &boundaries, string &charset, string &name)
{
    regex e_contenttype("^(Content-[Tt]ype:)(.*)");
    regex e_boundary1("^([ \t]*multipart/)(.*)");
    regex e_boundary2("^([a-zA-Z]+);[ \t]+(.*)");
    regex e_boundary3("^([ \t]*)(boundary=)\"?(.*)\"?;?(.*)");
    smatch sm;
    smatch sm1;
    smatch sm2;
    smatch sm_next;
    regex e_semi("[; \t]");
    smatch sm_semi;

    regex_match(lines.front(), sm, e_contenttype);
    if (!contenttype.length() && sm.size() > 0)
    {
        string ct = sm[2].str();
        trim(ct);

        for (auto it = lines.cbegin(); it != lines.cend(); ++it)
        {
            auto c = it->cbegin();
            if (*c == ' ' || *c == '\t')
                ct += *it;
        }

        string attr;
        istringstream iss(ct);
        getline(iss, attr, ';');
        trim(attr);
        contenttype = attr;
        while (getline(iss, attr, ';'))
        {
            trim(attr);
            size_t pos_eq = attr.find('=');
            string a = attr.substr(0, pos_eq);
            string v = attr.substr(pos_eq + 1);
            trim(v);
            strip_quotes(v);
            if (a == "charset")
                charset = str_tolower(v);
            else if (a == "type")
                contenttype = v;
            else if (a == "name")
                name = v;
            else if (a == "boundary")
                boundaries.push_back(v);
        }
    }
}

void EmailExtractor::extract_contentdisposition_elements(string s, string next, string &filename, string &cdate, string &mdate,
    int &sz)
{
    regex e_filename("^(.*)([ \t]*)(filename=)\"?(.*)\"?;?(.*)");
    regex e_size("^(.*)(size=)(.*);?(.*)");
    regex e_ctime("^(.*)(creation-date=)\"(.*)\";?(.*)");
    regex e_mtime("^(.*)(modification-date=)\"(.*)\";?(.*)");
    smatch sm;

    string size;

    regex_match(s, sm, e_filename);
    if (sm.size() > 0)
    {
        string fs = sm[4];
        smatch sm2;

        regex_match(fs, sm2, e_size);
        if (!sm2.size())
            filename = sm[4];
        else
            filename = sm2[1];

        // quite annoying that the quotes are optional
        trim(filename);
        strip_semi(filename);
        strip_quotes(filename);
    }

    regex_match(s, sm, e_size);
    if (sm.size() > 0)
    {
        size = sm[3];
        strip_semi(size);
        sz = stoi(size);
    }

    regex_match(s, sm, e_ctime);
    if (sm.size() > 0)
        cdate = sm[3];

    regex_match(s, sm, e_mtime);
    if (sm.size() > 0)
        mdate = sm[3];
}

int EmailExtractor::scan_headers(istream &infile,
    string &msgid,
    string &to,
    string &from,
    string &subject,
    string &date,
    string &contenttype,
    string &charset,
    string &transferenc,
    string &contentid,
    string &contentdisposition,
    BoundaryList &boundaries,
    Attachment &att)
{
    string prev = "", s = "", next = "";
    regex e_contenttype("^(Content-[Tt]ype:)(.*)");
    regex e_msgid("^(Message-[Ii][Dd]:).*<(.*)>(.*)");
    regex e_to("^(To:)[ \t]*(.*)");
    regex e_from("^(From:)[ \t]*(.*)");
    regex e_subject("^(Subject:)(.*)");
    regex e_date("^(Date:)(.*)");
    regex e_trenc("^(Content-Transfer-Encoding:)(.*)");
    regex e_disp("^(Content-Disposition:)(.*)");
    regex e_contentid("^(Content-[Ii][Dd]:).*<(.*)>(.*)");
    regex e_nexthdr("^([^ \t]+)(.*)");
    smatch sm;

    bool in_contenttype = false;
    bool in_content_disposition = false;
    bool in_multiline_subject = false;
    bool in_multiline_to = false;
    bool nexthdr = false;

    vector<string> contenttypelines;

    bool last = false;
    while (infile.good() || last)
    {
        getline(infile, next);

        // chomp off any cr left over from windows text
        auto c = next.cend();
        c--;
        if (*c == '\r')
            next.erase(c);

        regex_match(s, sm, e_nexthdr);
        if (sm.size() > 0)
            nexthdr = true;

        regex_match(s, sm, e_msgid);
        if (!msgid.length() && sm.size() > 0)
            msgid = sm[2];

        regex_match(s, sm, e_contentid);
        if (!contentid.length() && sm.size() > 0)
            contentid = sm[2];

        if (in_multiline_to)
        {
            if (nexthdr)
                in_multiline_to = false;
            else
            {
                string moreto = s;
                trim(moreto);
                to += moreto;
            }
        }
        regex_match(s, sm, e_to);
        if (sm.size() > 0)
        {
            to = sm[2];
            in_multiline_to = true;
        }

        regex_match(s, sm, e_from);
        if (!from.length() && sm.size() > 0)
            from = sm[2];

        if (in_multiline_subject)
        {
            if (nexthdr)
                in_multiline_subject = false;
            else
            {
                string moresubject = s;
                trim(moresubject);
                subject += moresubject;
            }
        }
        regex_match(s, sm, e_subject);
        if (sm.size() > 0)
        {
            subject = sm[2];
            auto c = subject.cbegin();
            if (*c == ' ')
                subject.erase(c);
            in_multiline_subject = true;
        }

        regex_match(s, sm, e_date);
        if (!date.length() && sm.size() > 0)
        {
            date = sm[2];
            trim(date);
        }

        regex_match(s, sm, e_trenc);
        if (!transferenc.length() && sm.size() > 0)
        {
            transferenc = sm[2];
            trim(transferenc);
        }

        regex_match(s, sm, e_contenttype);
        if (!contenttype.length() && sm.size() > 0)
        {
            in_contenttype = true;
            contenttypelines.insert(contenttypelines.cend(), s);
        }
        else if (in_contenttype)
        {

            if (!nexthdr && !last)
                contenttypelines.insert(contenttypelines.cend(), s);
            else
            {
                in_contenttype = false;
                extract_contenttype(contenttypelines, contenttype, boundaries, charset, att.m_attachment_filename);
                if (att.m_attachment_filename.length())
                    contentdisposition = "inline";
            }
        }

        regex_match(s, sm, e_disp);
        if (sm.size() > 0)
        {
            string tmp;
            int pos_semi;
            regex e_dtype("^(Content-Disposition:)([ \t]*)(.*);?(.*)");
            smatch smdt;

            regex_match(s, smdt, e_dtype);
            tmp = str_tolower(smdt[3].str());
            pos_semi = tmp.find(";");
            if (pos_semi != string::npos)
                contentdisposition = tmp.substr(0, pos_semi);
            else
                contentdisposition = tmp;

            if (str_tolower(contentdisposition) == "attachment" || str_tolower(contentdisposition) == "inline")
            {
                in_content_disposition = true;
                att.m_size = -1;
                att.m_attachment_filename.clear();
                att.m_creation_datetime.clear();
                att.m_modification_datetime.clear();
                extract_contentdisposition_elements(s, next, att.m_attachment_filename, att.m_creation_datetime,
                    att.m_modification_datetime, att.m_size);
            }
        }
        else if (!nexthdr && s.size() && in_content_disposition)
            extract_contentdisposition_elements(s, next, att.m_attachment_filename, att.m_creation_datetime,
                att.m_modification_datetime, att.m_size);
        else
            in_content_disposition = false;

        nexthdr = false;

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
        {
            if (in_contenttype)
            {
                extract_contenttype(contenttypelines, contenttype, boundaries, charset, att.m_attachment_filename);
                if (att.m_attachment_filename.length())
                    contentdisposition = "inline";
            }
            break;
        }
    }

    return 0;
}

int EmailExtractor::scan_headers(const string fname,
    string &msgid,
    string &to,
    string &from,
    string &subject,
    string &date,
    string &contenttype,
    string &charset,
    string &transferenc,
    string &contentid,
    string &contentdisposition,
    BoundaryList &boundaries,
    Attachment &att)
{
    ifstream infile(fname, ifstream::in);

    msgid = to = from = subject = date = contenttype = charset = transferenc = "";

    scan_headers(infile, msgid, to, from, subject, date, contenttype, charset, transferenc, contentid, contentdisposition, boundaries, att);

    infile.close();
    return 0;
}

int EmailExtractor::base64_encode(const vector<unsigned char> &input, vector<unsigned char> &output, bool preserve_crlf)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    if (BIO_write(b64, &input[0], input.size()) <= 0)
        return -1;

    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    for (int i; i < bptr->length; i++)
        if (preserve_crlf || (bptr->data[i] != '\r' && bptr->data[i] != '\n'))
            output.insert(output.end(), bptr->data[i]);

    BIO_free_all(b64);
    return 0;
}

int EmailExtractor::base64_decode(const vector<unsigned char> &input, vector<unsigned char> &output)
{
    BIO *b64, *bmem;
    int length = input.size();

    output.insert(output.end(), length, (char) 0);

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf(&input[0], length);
    bmem = BIO_push(b64, bmem);

    int i = 0;
    if ((i = BIO_read(bmem, &output[0], length)) <= 0)
        return -1;
    else
        output.resize(i);

    BIO_free_all(bmem);
    return 0;
}

int EmailExtractor::base64_decode(const string &input, string &output)
{
    vector<unsigned char> vinput(input.begin(), input.end());
    vector<unsigned char> voutput;
    if (base64_decode(vinput, voutput))
        return -1;

    output.clear();
    output.insert(output.end(), voutput.begin(), voutput.end());

    return 0;
}

int EmailExtractor::base64_encode(const string &input, string &output)
{
    vector<unsigned char> vinput(input.begin(), input.end());
    vector<unsigned char> voutput;
    if (base64_encode(vinput, voutput))
        return -1;

    output.clear();
    output.insert(output.end(), voutput.begin(), voutput.end());

    return 0;
}

int EmailExtractor::base64_decode(const string &input, vector<unsigned char> &output)
{
    vector<unsigned char> vinput(input.begin(), input.end());
    return base64_decode(vinput, output);
}

int EmailExtractor::base64_encode(const vector<unsigned char> &input, string &output)
{
    vector<unsigned char> voutput;
    if (base64_decode(input, voutput))
        return -1;

    output.clear();
    output.insert(output.end(), voutput.begin(), voutput.end());

    return 0;
}

int EmailExtractor::quoted_printable_decode(const std::string &input, std::string &output)
{
    auto c = input.cbegin();
    output.clear();

    while (c != input.cend())
    {
        if (*c == '=')
        {
            char c1, c2;
            c++;
            c1 = *c;
            c++;
            c2 = *c;
            if (!(c1 == '\r' && c2 == '\n'))
            {
                ostringstream o;
                char *stop;
                string hex = "0x";
                hex += c1;
                hex += c2;
                o << (char) strtol(hex.c_str(), &stop, 16);
                string s = o.str();
                output.insert(output.end(), s.cbegin(), s.cend());
            }
        }
        else
            output.insert(output.end(), *c);

        c++;
    }
    return 0;
}

string EmailExtractor::str_tolower(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
    {   return tolower(c);});
    return s;
}

string EmailExtractor::str_toupper(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
    {   return toupper(c);});
    return s;
}

bool EmailExtractor::is_utf8(string charset)
{
    charset = str_tolower(charset);
    return ((charset == "utf8" || charset == "utf-8"));
}

bool EmailExtractor::read_istream_to_boundary(istream &infile, BoundaryList &boundaries, vector<unsigned char> &rawbody, bool strip_crlf)
{
    string next;
    bool reached_prev_boundary = false;

    string boundary = "";
    if (boundaries.size())
        boundary = boundaries.back();

    string prev_boundary = "";
    if (boundaries.size() > 1)
    {
        auto bi = boundaries.end();
        bi--;
        bi--;
        prev_boundary = *bi;
    }


    string b = "--" + boundary;
    string end_b = b + "--";
    string prev_b = "--" + prev_boundary;

    while (infile.good())
    {
        getline(infile, next);
        // chomp off any cr left over from windows text
        auto c = next.cend();
        c--;
        if (*c == '\r')
            next.erase(c);

        if (boundary.length() && !next.compare(0, b.length(), b))
        {
            // we have reached the boundary.  Is it the last of its kind?
            if (!next.compare(0, end_b.length(), end_b))
            {
                // yes it is
                //cout << "Reached last of " << boundaries.back() << endl;;
                boundaries.pop_back();
            }
            break;
        }

        if (prev_boundary.length() && prev_boundary.length())
        {
            if (!next.compare(0, prev_b.length(), prev_b))
            {
                reached_prev_boundary = true;
                break;
            }
        }

        if (!strip_crlf)
            next.append("\r\n");

        rawbody.insert(rawbody.end(), next.cbegin(), next.cend());
    }

    return reached_prev_boundary;
}
