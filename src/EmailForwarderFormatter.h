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
    EmailForwarderFormatter(std::string from, email_list destinations, outgoing_mail_server server_info, Aws::String dir, int verbose = 0);
    EmailForwarderFormatter(std::string from, email_list destinations, outgoing_mail_server server_info, Aws::String dir, mxbypref mxservers, int verbose = 0);
    virtual ~EmailForwarderFormatter();

    virtual void clean_up();

private:
    void init();
    std::string formatFile(std::string to, std::string fullpath);
    void forwardFile(std::string env_to, std::string email);

    std::string newBoundary();

    static std::string date();

    std::string m_from;
    std::string m_domain;
    email_list m_destinations;
    outgoing_mail_server m_server_info;
    int m_verbose;
};

#endif /* SRC_EmailForwarderFORMATTER_H_ */
