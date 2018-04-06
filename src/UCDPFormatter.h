/*
 * UCDPFormatter.h
 *
 *  Created on: Oct 29, 2017
 *      Author: Paul Cifarelli
 *
 *      The UCDPFormatter is responsible for formatting and posting emails to UCDP in EmailML format
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
#include "EmailExtractor.h"

class UCDPFormatter: public S3Downloader::Formatter
{
public:
    UCDPFormatter(
        Aws::String workdir,
        std::string email_addr,
        std::string ip,
        std::string snihostname,
        unsigned short port,
        std::string certificate,
        std::string certpassword,
        std::string trclientid,
        std::string trfeedid,
        std::string trmessagetype,
        int trmessageprio,
        bool validate_json,
        int verbose = 0);
    UCDPFormatter(
        Aws::String workdir,
        mxbypref    mxservers,
        std::string email_addr,
        std::string ip,
        std::string snihostname,
        unsigned short port,
        std::string certificate,
        std::string certpassword,
        std::string trclientid,
        std::string trfeedid,
        std::string trmessagetype,
        int trmessageprio,
        bool validate_json,
        int verbose = 0);

    virtual ~UCDPFormatter();
    virtual void open(const Aws::S3::Model::Object obj, const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    virtual void clean_up();

    static std::string escape_json(const std::string &s);
    static std::string unescape_json(const std::string &s);

private:
    void postToUCDP(std::string msgid, std::string date, EmailExtractor *email, bool validate = true);
    std::string fmtToField( std::string csstr );
    std::string formatMessage1(std::ostringstream &o, std::string msgid, std::string date, EmailExtractor *email, bool validate);
    std::string formatMessage2(std::ostringstream &o, std::string msgid, std::string date, EmailExtractor *email, bool validate);
    std::string formatMessage3(std::ostringstream &o, std::string msgid, std::string date, EmailExtractor *email, bool validate);
    std::string formatMessage4(std::ostringstream &o, int num, std::string msgid, std::string date, EmailExtractor::Body &body, bool validate);
    std::string formatMessage5(std::ostringstream &o, int num, std::string msgid, std::string date, EmailExtractor::Attachment &attachment, bool validate);
    void check_result(std::string result, std::string trmsgid);

    void recover();
    void processEmail(std::string fname);
    void unstash();

    std::string m_objkey;
    Aws::Utils::DateTime m_message_drop_time;
    int m_verbose;
    bool m_validate_json;
    int m_trmessageprio;
    std::string m_ip;
    std::string m_certificate;
    std::string m_certpassword;
    std::string m_snihostname;
    unsigned short m_port;
    std::string m_trclientid;
    std::string m_trfeedid;
    std::string m_trmessagetype;
    std::string m_to;
};

#endif /* SRC_UCDPFORMATTER_H_ */
