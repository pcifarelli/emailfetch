/*
 * EmailForwarderFormatter.h
 *
 *  Created on: Apr 7, 2018
 *      Author: Paul Cifarelli
 *
 *      This class is responsible for forwarding a received email to a user mailbox (client forwarding)
 */

#ifndef SRC_EmailForwarderFORMATTER_H_
#define SRC_EmailForwarderFORMATTER_H_

#include <stdlib.h>
#include <aws/core/Aws.h>
#include <aws/s3/model/Object.h>
#include "EmailFetcherConfig.h"
#include "Formatter.h"
#include "SMTPSender.h"

class EmailForwarderFormatter: public S3Downloader::Formatter
{
public:
    EmailForwarderFormatter(std::string from, email_list destinations, Aws::String dir, int verbose = 0);
    EmailForwarderFormatter(std::string from, email_list destinations, Aws::String dir, mxbypref mxservers, int verbose = 0);
    virtual ~EmailForwarderFormatter();

    virtual void clean_up();

private:
    void init();
    void formatFile(std::string to, std::string fullpath, std::string formattedFile);
    void forwardFile(std::string to, std::string formattedFile);

    static std::string date();

    std::string m_from;
    email_list m_destinations;
    int m_verbose;
};

#endif /* SRC_EmailForwarderFORMATTER_H_ */
