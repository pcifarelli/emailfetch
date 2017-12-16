/*
 * UCDPEmailExtractor.cpp
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

#include "UCDPEmailExtractor.h"

using namespace std;

UCDPEmailExtractor::UCDPEmailExtractor(const string fullpath) :
    m_fullpath(fullpath)
{
    std::string boundary;
    std::string contentdisposition;
    Attachment att;
    
    scan_headers(m_fullpath.c_str(), m_msgid, m_to, m_from, m_subject, m_date, m_contenttype, boundary, m_charset, m_transferenc, contentdisposition, att);
    save_parts(m_fullpath.c_str());
}

UCDPEmailExtractor::~UCDPEmailExtractor()
{
    // TODO Auto-generated destructor stub
}

int UCDPEmailExtractor::to_utf8(string charset, vector<unsigned char> &in, vector<unsigned char> &out)
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
	free (pout);
        return -1;
    }
    iconv_close(cd);
    size_t cnvtd = (size_t) (pout_iconv - pout);

    for (int i; i < cnvtd; i++)
        out.insert(out.end(), pout[i]);

    free(pout);

    return 0;
}

void UCDPEmailExtractor::transform_body(vector<unsigned char> &rawbody, string transferenc, string &charset, string &body)
{
    // convert everything to utf-8
    if (str_tolower(transferenc) == "base64" && !is_utf8(charset))
    {
        vector<unsigned char> vbody, uvbody;
        base64_decode(rawbody, vbody);
        if (!to_utf8(charset, vbody, uvbody))
            charset = "utf-8";
        base64_encode(uvbody, rawbody);
    }
    else if (!is_utf8(charset))
    {
        vector<unsigned char> vbody(rawbody.cbegin(), rawbody.cend());
        if (!to_utf8(charset, vbody, rawbody))
            charset = "utf-8";
    }

    body.insert(body.end(), rawbody.cbegin(), rawbody.cend());
}

bool UCDPEmailExtractor::extract_body(ifstream &infile, string contenttype, string transferenc, string &charset, string &body)
{
    vector<unsigned char> rawbody;

    read_ifstream(infile, rawbody);
    body.clear();

    if (!contenttype.compare(0, 4, "text"))
	transform_body(rawbody, transferenc, charset, body);
    else
	body.insert(body.end(), rawbody.cbegin(), rawbody.cend());

    return false;
}

bool UCDPEmailExtractor::extract_body(ifstream &infile, string contenttype, string prev_boundary, string boundary, string transferenc, string &charset, string &body)
{
    bool is_text = !contenttype.compare(0, 4, "text");
    bool strip_crlf = (str_tolower(transferenc) == "base64");
    vector<unsigned char> rawbody;
    bool reached_prev_boundary;

    rawbody.clear();
    reached_prev_boundary = read_ifstream_to_boundary(infile, prev_boundary, boundary, rawbody, strip_crlf);

    body.clear();
    if (rawbody.size() && is_text)
        transform_body(rawbody, transferenc, charset, body);
    else
	body.insert(body.end(), rawbody.cbegin(), rawbody.cend());

    return reached_prev_boundary;
}

int UCDPEmailExtractor::scan_attachment_headers(ifstream &infile, string &contenttype, string &boundary, string &charset,
						string &transferenc, string &contentdisposition, Attachment &att)
{
    string dummy;
    string a_contenttype, a_boundary, a_charset, a_transferenc, a_contentdisposition;
    Attachment a_att;
    contenttype.clear();
    boundary.clear();
    charset.clear();
    transferenc.clear();
    contentdisposition.clear();
    att.m_attachment_filename.clear();
    att.m_creation_datetime.clear();
    att.m_modification_datetime.clear();
    att.m_size = 0;
    int ret = scan_headers(infile, dummy, dummy, dummy, dummy, dummy, a_contenttype, a_boundary, a_charset, a_transferenc, a_contentdisposition, a_att);
    if (a_contenttype.length())
        contenttype = a_contenttype;
    if (a_boundary.length())
        boundary = a_boundary;
    if (a_charset.length())
        charset = a_charset;
    if (a_transferenc.length())
        transferenc = a_transferenc;
    if (a_contentdisposition.length())
    {
	contentdisposition = a_contentdisposition;
	if (str_tolower(contentdisposition) == "attachment")
	    att = a_att;
    }
}

bool UCDPEmailExtractor::extract_attachments(ifstream &infile, string prev_boundary, string contenttype, string boundary,
					     string charset, string transferenc)
{
    string a_contenttype = contenttype, a_boundary = boundary, a_charset = charset, a_transferenc = transferenc;
    bool reached_prev_boundary = false;
    string contentdisposition = "";
    Attachment att;

    while (infile.good() || reached_prev_boundary)
    {
        string a_filename;

	if (str_tolower(contentdisposition) == "attachment")
	{
	    reached_prev_boundary = extract_body(infile, a_contenttype, prev_boundary, boundary, a_transferenc, a_charset, att.m_attachment);
	    att.m_contenttype = a_contenttype;
	    att.m_charset = a_charset;
	    att.m_transferenc = a_transferenc;

	    m_attachments.push_back(att);	    
	}
	else
	{
	    Body b;
	    reached_prev_boundary = extract_body(infile, a_contenttype, prev_boundary, boundary, a_transferenc, a_charset, b.m_body);

	    // if it has some size, it's not just a newline, and it's content type is not multipart, then we save it
	    if (b.m_body.size() && b.m_body != "\r\n" && a_contenttype.compare(0, 9, "multipart"))
	    {
		b.m_contenttype = a_contenttype;
		b.m_charset = a_charset;
		b.m_transferenc = a_transferenc;

		m_bodies.push_back(b);
	    }
	}
	
        if (reached_prev_boundary)
        {
            // we were extracting a subdocument
            return reached_prev_boundary;
        }

        // see if there are new headers to deal with
        scan_attachment_headers(infile, a_contenttype, a_boundary, a_charset, a_transferenc, contentdisposition, att);

        if (a_boundary.length() && (a_boundary != boundary))
        {
            extract_attachments(infile, boundary, a_contenttype, a_boundary, a_charset, a_transferenc);

            // we returned because we reached the prev boundary, so we are possibly pointing to attchment headers, so check
            scan_attachment_headers(infile, a_contenttype, a_boundary, a_charset, a_transferenc, contentdisposition, att);
        }
    }

    return reached_prev_boundary;
}

int UCDPEmailExtractor::save_parts(const string fname)
{
    ifstream infile(fname, ifstream::in);
    string msgid, to, from, subject, date, contenttype, boundary, charset, transferenc, contentdisposition;
    Attachment att;

    // this version of scan_headers leaves the ifstream pointing to the first line after the header block
    scan_headers(infile, msgid, to, from, subject, date, contenttype, boundary, charset, transferenc, contentdisposition, att);

    if (!contenttype.compare(0, 9, "multipart"))
    {
        // we have attachments and/or multiple bodies to deal with
        extract_attachments(infile, "", contenttype, boundary, charset, transferenc);
    }
    else
    {
	Body b;
	
        // simple email
        b.m_contenttype = contenttype;
        b.m_charset = charset;
	b.m_transferenc = transferenc;
        extract_body(infile, contenttype, transferenc, charset, b.m_body);
	m_bodies.push_back(b);
    }
}

// trim from start (in place)
void UCDPEmailExtractor::ltrim(string &s)
{
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch)
    {
        return !isspace(ch);
    }));
}

// trim from end (in place)
void UCDPEmailExtractor::rtrim(string &s)
{
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch)
    {
        return !isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void UCDPEmailExtractor::trim(string &s)
{
    ltrim(s);
    rtrim(s);
}

string UCDPEmailExtractor::strip_semi(string &s)
{
    // strip the semi-colon if its there
    auto c = s.cend();
    c--;
    if (*c == ';')
        s.erase(c);

    return s;
}

string UCDPEmailExtractor::strip_quotes(string &s)
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

string UCDPEmailExtractor::extract_attr(string attr, string line)
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

void UCDPEmailExtractor::extract_contenttype(string s, string next, string &contenttype, string &boundary, string &charset)
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

void UCDPEmailExtractor::extract_contentdisposition_elements(string s, string next, string &filename, string &cdate, string &mdate,
    int &sz)
{
    regex e_filename("^(.*)([ \t]*)(filename=)\"(.*)\";?(.*)");
    regex e_size("^(.*)(size=)(.*);?(.*)");
    regex e_ctime("^(.*)(creation-date=)\"(.*)\";?(.*)");
    regex e_mtime("^(.*)(modification-date=)\"(.*)\";?(.*)");
    smatch sm;

    string size;

    regex_match(s, sm, e_filename);
    if (sm.size() > 0)
        filename = sm[4];

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

int UCDPEmailExtractor::scan_headers(ifstream &infile, string &msgid, string &to, string &from, string &subject, string &date,
				     string &contenttype, string &boundary, string &charset, string &transferenc, string &contentdisposition,
				     Attachment &att)
{
    string prev = "", s = "", next = "";
    regex e_msgid("^(Message-ID:).*<(.*)>(.*)");
    regex e_to("^(To:)[ \t]*<?(.*)>?");
    regex e_from("^(From:)[ \t]*<?(.*)>?");
    regex e_subject("^(Subject:)(.*)");
    regex e_date("^(Date:)(.*)");
    regex e_trenc("^(Content-Transfer-Encoding:)(.*)");
    regex e_disp("^(Content-Disposition:)(.*)");
    regex e_nexthdr("^([^ \t]+)(.*)");
    smatch sm;

    bool in_content_disposition = false;
    bool nexthdr = false;

    bool last = false;
    while ((infile.good() || last)
        && !(msgid.length() && to.length() && from.length() && subject.length() && date.length() && contenttype.length()
            && transferenc.length()))
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

            if (contentdisposition == "attachment")
            {
                in_content_disposition = true;
                extract_contentdisposition_elements(s, next, att.m_attachment_filename, att.m_creation_datetime, att.m_modification_datetime, att.m_size);
            }
        }
        else if (!nexthdr && s.size() && in_content_disposition)
            extract_contentdisposition_elements(s, next, att.m_attachment_filename, att.m_creation_datetime, att.m_modification_datetime, att.m_size);
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
            break;
    }

}

int UCDPEmailExtractor::scan_headers(const string fname, string &msgid, string &to, string &from, string &subject, string &date,
				     string &contenttype, string &boundary, string &charset, string &transferenc, string &contentdisposition,
				     Attachment &att)
{
    ifstream infile(fname, ifstream::in);

    msgid = to = from = subject = date = contenttype = boundary = charset = transferenc = "";

    scan_headers(infile, msgid, to, from, subject, date, contenttype, boundary, charset, transferenc, contentdisposition, att);

    infile.close();
    return 0;
}

void UCDPEmailExtractor::base64_encode(vector<unsigned char> &input, vector<unsigned char> &output, bool preserve_crlf)
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

void UCDPEmailExtractor::base64_decode(vector<unsigned char> &input, vector<unsigned char> &output)
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
        cout << "base64 decode failed length=" << length << endl;
    else
        output.resize(i);

    BIO_free_all(bmem);
}

string UCDPEmailExtractor::str_tolower(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
    {   return tolower(c);});
    return s;
}

string UCDPEmailExtractor::str_toupper(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
    {   return toupper(c);});
    return s;
}

bool UCDPEmailExtractor::is_utf8(string charset)
{
    charset = str_tolower(charset);
    return ((charset == "utf8" || charset == "utf-8"));
}

void UCDPEmailExtractor::read_ifstream(ifstream &infile, vector<unsigned char> &rawbody)
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

bool UCDPEmailExtractor::read_ifstream_to_boundary(ifstream &infile, string prev_boundary, string boundary,
						   vector<unsigned char> &rawbody, bool strip_crlf)
{
    string next, b = "^(--" + boundary + ")(.*)";
    string prev_b = "^(--" + prev_boundary + ")(.*)";
    bool reached_prev_boundary = false;
    regex e_b(b);
    regex e_prev_b(prev_b);
    smatch sm;
    smatch psm;

    while (infile.good())
    {
        getline(infile, next);
        // chomp off any cr left over from windows text
        auto c = next.cend();
        c--;
        if (*c == '\r')
            next.erase(c);

        regex_match(next, sm, e_b);
        if (sm.size() > 0)
            break;

        if (prev_boundary.length())
        {
            regex_match(next, psm, e_prev_b);
            if (psm.size() > 0)
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
