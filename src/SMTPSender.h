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

class SMTPSender
{
public:
    SMTPSender(std::string smtpserver);
    virtual ~SMTPSender();

    int send(std::string email, std::string to, std::string from);
    int sendFile(std::string fname, std::string to, std::string from);

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

    static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp);
};

#endif /* SRC_SMTPSENDER_H_ */
