/*
 * SMTPSender.h
 *
 *  Created on: Jan 5, 2018
 *      Author: Paul Cifarelli
 *
 *      Forward/Send an email to an SMTP server
 */

#ifndef SRC_SMTPSENDER_H_
#define SRC_SMTPSENDER_H_

#include <string>
#include "EmailExtractor.h"

class SMTPSender
{
public:
    SMTPSender(std::string smtpserver);
    virtual ~SMTPSender();

    int send(std::string &email, std::string to, std::string from);
    int sendFile(std::string fname, std::string to, std::string from);

    // the only difference between the forward functions and the send functions
    // is that the forward functions use the Return-Path as the envelope "from"
    // if it's in the header, and make sure there is a Return-Path header if it
    // isn't there, in which case it uses the "From:" header as the "from"
    // envelope and adds a Return-Path set to the same value (note that the latter
    // is not always reliable, so a warning is printed if it's done)
    int forward(std::string &email, std::string to);
    int forwardFile(std::string fname, std::string to);
    int forward(std::string &email);
    int forwardFile(std::string fname);

    CURLcode status() const
    {
        return m_curl_status;
    }

private:
    std::string m_smtpserver;
    std::string m_smtpurl;
    std::string m_result;

    CURLcode m_curl_status;
    
    struct WriteThis
    {
        const char *readptr;
        int sizeleft;
    };

    std::string pick(std::string      &to,
                     EmailExtractor::ToSet &original_to,
                     EmailExtractor::ToSet &delivered_to,
                     EmailExtractor::ToSet &envelope_to,
                     std::string           &returnpath,
                     std::string           &envelope_from,
                     std::string           &from,
                     std::string           &env_from,
                     EmailExtractor::ToSet &env_to);

    static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp);
};

#endif /* SRC_SMTPSENDER_H_ */
