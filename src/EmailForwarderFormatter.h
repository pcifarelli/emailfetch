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
#include "Formatter.h"
#include "SMTPSender.h"

class EmailForwarderFormatter: public S3Downloader::Formatter
{
public:
    EmailForwarderFormatter(Aws::String to, Aws::String from, Aws::String outgoingserver, Aws::String username, Aws::String password, unsigned short port, bool tls,
        int verbose = 0);
    EmailForwarderFormatter(Aws::String to, Aws::String from, Aws::String outgoingserver, Aws::String username, Aws::String password, unsigned short port, bool tls,
        mxbypref mxservers, int verbose = 0);
    EmailForwarderFormatter(Aws::String to, Aws::String from, Aws::String outgoingserver, Aws::String username, Aws::String password, unsigned short port, bool tls,
        Aws::String dir, int verbose = 0);
    EmailForwarderFormatter(Aws::String to, Aws::String from, Aws::String outgoingserver, Aws::String username, Aws::String password, unsigned short port, bool tls,
        Aws::String dir, mxbypref mxservers, int verbose = 0);
    virtual ~EmailForwarderFormatter();

    virtual void clean_up();

private:
    std::string m_to;
    std::string m_from;
    std::string m_outgoingserver;
    std::string m_username;
    std::string m_password;
    unsigned short m_port;
    bool m_tls;
    int m_verbose;
};

#endif /* SRC_EmailForwarderFORMATTER_H_ */
