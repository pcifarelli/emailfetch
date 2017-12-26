/*
 * EmailExtractor.h
 *
 *  Created on: Dec 10, 2017
 *      Author: paulc
 *
 * Parses an email into it's bodies and attachments, extracting relevant information like it's charset and transfer encoding
 * extracts the whole email into memory, so be careful of huge attachments
 */

#ifndef SRC_EMAILEXTRACTOR_H_
#define SRC_EMAILEXTRACTOR_H_

#include <sstream>
#include <iomanip>
#include <regex>
#include <list>
#include <vector>

class EmailExtractor
{
public:
    EmailExtractor(std::string fullpath);
    virtual ~EmailExtractor();

    enum ERRORS
    {
        NO_ERROR,
        ERROR_B64ENCODE,
        ERROR_B64DECODE,
	ERROR_QPDECODE,
        ERROR_UTF8CONV,
        ERROR_NOOPEN,
        NUM_ERRORS
    };

    class Body
    {
    public:
        // as received, transfer encoding as indicated by transferenc() and charset as indicated by charset()
        const std::string &body();
        const std::string &contenttype();
        const std::string &charset();
        const std::string &transferenc();
	const std::string &contentID();
        std::string asBase64();
        std::string asUtf8();
        // beware using this on text/html as it may have embedded charset definitions
        std::string asUtf8Base64();
        int error() const;
        std::string str_error() const;

    private:
        std::string m_body;
        std::string m_contenttype;
        std::string m_charset;
        std::string m_transferenc;
	std::string m_contentid;
        std::string m_str_error;
        int m_error;

        friend class EmailExtractor;
    };
    typedef std::list<Body> BodyList;

    class Attachment
    {
    public:
        const std::string &attachment();
        const std::string &filename();
        const std::string &creation_time();
        const std::string &modification_time();
        const std::string &contenttype();
        const std::string &charset();
        const std::string &transferenc();
        const int size();
        void save_attachment(std::string path);
        std::string asBase64();
        int error() const;
        std::string str_error() const;

    private:
        std::string m_attachment;
        std::string m_attachment_filename;
        std::string m_creation_datetime;
        std::string m_modification_datetime;
        std::string m_contenttype;
        std::string m_charset;
        std::string m_transferenc;
        int m_size;
        std::string m_str_error;
        int m_error;

        friend class EmailExtractor;
    };
    typedef std::list<Attachment> AttachmentList;

    const std::string &msgid();
    const std::string &to();
    const std::string &from();
    const std::string &subject();
    const std::string &date();
    const std::string &contenttype();
    const std::string &charset();
    const std::string &transferenc();

    int num_bodies();
    int num_attachments();
    BodyList &bodies();
    AttachmentList &attachments();

    int error() const;
    std::string str_error() const;

    static int base64_decode(const std::string &input, std::string &output);
    static int base64_encode(const std::string &input, std::string &output);
    static int base64_decode(const std::string &input, std::vector<unsigned char> &output);
    static int base64_encode(const std::vector<unsigned char> &input, std::string &output);
    static int base64_decode(const std::vector<unsigned char> &input, std::vector<unsigned char> &output);
    static int base64_encode(const std::vector<unsigned char> &input, std::vector<unsigned char> &output, bool preserve_crlf = false);

    static int quoted_printable_decode(const std::string &input, std::string &output);

    // some convenience functions
    static bool is_utf8(std::string charset);
    static int to_utf8(std::string charset, std::vector<unsigned char> &in, std::vector<unsigned char> &out);
    static std::string str_tolower(std::string s);
    static std::string str_toupper(std::string s);
    static void ltrim(std::string &s);
    static void rtrim(std::string &s);
    static void trim(std::string &s);
    static std::string strip_semi(std::string &s);
    static std::string strip_quotes(std::string &s);

private:
    int m_error;
    std::string m_str_error;

    std::string m_fullpath;
    std::string m_msgid;
    std::string m_to;
    std::string m_from;
    std::string m_subject;
    std::string m_date;
    std::string m_contenttype;
    std::string m_charset;
    std::string m_transferenc;

    BodyList m_bodies;
    AttachmentList m_attachments;

    // saves bodies and attachments in memory
    int save_parts(const std::string fname);

    // reads up to the boundary (defined as "--" + boundary, as per rfc2046)
    static bool read_ifstream_to_boundary(std::ifstream &infile, std::string prev_boundary, std::string boundary,
        std::vector<unsigned char> &rawbody, bool strip_crlf = true);

    // the heavy lifting functions
    static const int UTF8_MAX = 6;
    static int scan_headers(const std::string fname,
        std::string &msgid,
        std::string &to,
        std::string &from,
        std::string &subject,
        std::string &date,
        std::string &contenttype,
        std::string &boundary,
        std::string &charset,
        std::string &transferenc,
	std::string &contentid,
        std::string &contentdispositon,
        Attachment &attachment);

    static int scan_headers(std::ifstream &f,
        std::string &msgid,
        std::string &to,
        std::string &from,
        std::string &subject,
        std::string &date,
        std::string &contenttype,
        std::string &boundary,
        std::string &charset,
        std::string &transferenc,
	std::string &contentid,
        std::string &contentdisposition,
        Attachment &attachment);

    static int scan_attachment_headers(std::ifstream &infile,
        std::string &contenttype,
        std::string &boundary,
        std::string &charset,
        std::string &transferenc,
	std::string &contentid,
        std::string &contentdisposition,
        Attachment &attachment);

    bool extract_all(std::ifstream &infile,
        std::string prev_boundary,
        std::string contenttype,
        std::string boundary,
        std::string charset,
        std::string transferenc);

    bool extract_body(std::ifstream &infile,
        std::string contenttype,
        std::string prev_boundary,
        std::string boundary,
        std::string transferenc,
        std::string &charset,
        std::string &body);

    static std::string extract_attr(std::string attr, std::string line);

    static void extract_contentdisposition_elements(std::string s,
        std::string next,
        std::string &filename,
        std::string &cdate,
        std::string &mdate,
        int &sz);

    static void extract_contenttype(std::vector<std::string> &lines,
        std::string &contenttype,
        std::string &boundary,
	std::string &charset,
	std::string &name);

};

#endif /* SRC_EMAILEXTRACTOR_H_ */
