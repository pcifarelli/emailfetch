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

private:
    std::string m_objkey;
    Aws::Utils::DateTime m_message_drop_time;
};

#endif /* SRC_UCDPFORMATTER_H_ */
