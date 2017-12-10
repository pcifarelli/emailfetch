/*
 * UCDPEmailExtractor.h
 *
 *  Created on: Dec 10, 2017
 *      Author: paulc
 */

#ifndef SRC_UCDPEMAILEXTRACTOR_H_
#define SRC_UCDPEMAILEXTRACTOR_H_

#include <sstream>
#include <iomanip>
#include <regex>
#include <list>
#include <vector>

class UCDPEmailExtractor
{
public:
    UCDPEmailExtractor(std::string fullpath);
    virtual ~UCDPEmailExtractor();

    const std::string &msgid()       const { return m_msgid; }
    const std::string &to()          const { return m_to; }
    const std::string &from()        const { return m_from; }
    const std::string &subject()     const { return m_subject; }
    const std::string &date()        const { return m_date; }
    const std::string &contenttype() const { return m_contenttype; }
    const std::string &charset()     const { return m_charset; }
    const std::string &transferenc() const { return m_transferenc; }

private:
    std::string m_fullpath;
    std::string m_msgid;
    std::string m_to;
    std::string m_from;
    std::string m_subject;
    std::string m_date;
    std::string m_contenttype;
    std::string m_charset;
    std::string m_transferenc;

    struct Body
    {
        std::string local_path;
        std::string contenttype;
        std::string charset;
        std::string transferenc;
    };

    struct Attachment
    {
        std::string attachment_filename;
        std::string creation_datetime;
        std::string modification_datetime;
        int size;
        std::string local_path;
    };
    typedef std::list<Attachment> AttachmentList;

    AttachmentList m_attachments;

    static const int UTF8_MAX = 6;
    static int scan_headers(const std::string fname, std::string &msgid, std::string &to, std::string &from, std::string &subject,
        std::string &date, std::string &contenttype, std::string &boundary, std::string &charset, std::string &transferenc);

    static int scan_headers(std::ifstream &f, std::string &msgid, std::string &to, std::string &from, std::string &subject,
        std::string &date, std::string &contenttype, std::string &boundary, std::string &charset, std::string &transferenc);

    // reads to the end of file
    static void read_ifstream(std::ifstream &infile, std::vector<unsigned char> &rawbody);
    // reads up to the boundary (defined as "--" + boundary, as per rfc2046)
    static bool read_ifstream_to_boundary(std::ifstream &infile, std::string prev_boundary, std::string boundary, std::vector<unsigned char> &rawbody);

    static inline bool is_utf8(std::string charset);
    static int to_utf8(std::string charset, std::vector<unsigned char> &in, std::vector<unsigned char> &out);
    static inline std::string str_tolower(std::string s);
    static inline std::string str_toupper(std::string s);
    static void base64_decode(std::vector<unsigned char> &input, std::vector<unsigned char> &output);
    static void base64_encode(std::vector<unsigned char> &input, std::vector<unsigned char> &output, bool preserve_crlf = false);
    static inline void ltrim(std::string &s);
    static inline void rtrim(std::string &s);
    static inline void trim(std::string &s);
    static inline std::string strip_semi(std::string &s);
    static inline std::string strip_quotes(std::string &s);

    static int  save_parts(const std::string fname, const std::string savedir);
    static bool extract_attachments(std::ifstream &infile, std::string prev_boundary, std::string contenttype, std::string boundary, std::string charset, std::string transferenc);
    static int  scan_attachment_headers(std::ifstream &infile, std::string &contenttype, std::string &boundary, std::string &charset, std::string &transferenc, std::string &attachment_filename);
    static bool extract_body(std::ifstream &infile, std::string prev_boundary, std::string boundary, std::string transferenc, std::string &charset, std::string &body);
    static bool extract_body(std::ifstream &infile, std::string transferenc, std::string &charset, std::string &body);
    static void transform_body(std::vector<unsigned char> &rawbody, std::string transferenc, std::string &charset, std::string &body);

    static std::string extract_attr(std::string attr, std::string line);
    static void extract_contentdisposition_elements(std::string s, std::string next, std::string &filename, std::string &cdate, std::string &mdate, int &sz);

    static void extract_contenttype(std::string s, std::string next, std::string &contenttype, std::string &boundary,
        std::string &charset);

};

#endif /* SRC_UCDPEMAILEXTRACTOR_H_ */
