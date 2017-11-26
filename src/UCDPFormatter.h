/*
 * UCDPFormatter.h
 *
 *  Created on: Oct 29, 2017
 *      Author: paulc
 */

#ifndef SRC_UCDPFORMATTER_H_
#define SRC_UCDPFORMATTER_H_

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <sstream>
#include <iomanip>
#include <regex>
#include "Formatter.h"
#include "UCDPCurlPoster.h"

class UCDPFormatter: public S3Downloader::Formatter
{
public:
    UCDPFormatter(Aws::String workdir);
    virtual ~UCDPFormatter();
    virtual void open(const Aws::S3::Model::Object obj, const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    virtual void clean_up();

    static std::string escape_json(const std::string &s);
    static std::string unescape_json(const std::string &s);

    static int scan_headers(const std::string fname, std::string &msgid, std::string &to, std::string &from, std::string &subject,
        std::string &date, std::string &contenttype, std::string &boundary, std::string &charset, std::string &transferenc);

    static int scan_headers(std::ifstream &f, std::string &msgid, std::string &to, std::string &from, std::string &subject,
        std::string &date, std::string &contenttype, std::string &boundary, std::string &charset, std::string &transferenc);

private:
    static void read_ifstream(std::ifstream &infile, std::vector<unsigned char> &rawbody);
    static void read_ifstream1(std::ifstream &infile, std::vector<unsigned char> &rawbody);
    static inline bool is_utf8(std::string charset);
    static inline std::string str_tolower(std::string s);
    static void base64_decode(std::vector<unsigned char> &input, std::vector<unsigned char> &output);
    static void base64_encode(std::vector<unsigned char> &input, std::vector<unsigned char> &output);
    static inline void ltrim(std::string &s);
    static inline void rtrim(std::string &s);
    static inline void trim(std::string &s);

    static inline std::string strip_semi(std::string &s);
    static std::string extract_attr(std::string attr, std::string line);
    static void extract_contenttype(std::string s, std::string next, std::string &contenttype, std::string &boundary,
        std::string &charset);
    static int scan_attachment_headers(std::ifstream &f, std::string &contenttype, std::string &boundary, std::string &charset,
        std::string &transferenc, std::string &attachment_filename);

    std::string m_objkey;
    Aws::Utils::DateTime m_message_drop_time;
};

#endif /* SRC_UCDPFORMATTER_H_ */
