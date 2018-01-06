/*
 * SMTPSender.cpp
 *
 *  Created on: Jan 5, 2018
 *      Author: paulc
 */

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <curl/curl.h>

#include "SMTPSender.h"

using namespace std;

SMTPSender::SMTPSender(const string smtpserver) : m_smtpserver(smtpserver)
{
    m_smtpurl = "smtp://" + smtpserver;

    /* In windows, this will init the winsock stuff */
    m_curl_status = curl_global_init(CURL_GLOBAL_DEFAULT);

    /* Check for errors */
    if (m_curl_status != CURLE_OK)
        cout << "curl_global_init() failed: " << curl_easy_strerror(m_curl_status) << endl;
}

SMTPSender::~SMTPSender()
{
    curl_global_cleanup();
}

size_t SMTPSender::read_callback(void *dest, size_t size, size_t nmemb, void *userp)
{
    struct WriteThis *wt = (struct WriteThis *) userp;
    size_t buffer_size = size * nmemb;

    if (wt->sizeleft)
    {
        /* copy as much as possible from the source to the destination */
        size_t copy_this_much = wt->sizeleft;
        if (copy_this_much > buffer_size)
            copy_this_much = buffer_size;
        memcpy(dest, wt->readptr, copy_this_much);

        wt->readptr += copy_this_much;
        wt->sizeleft -= copy_this_much;
        return copy_this_much; /* we copied this many bytes */
    }

    return 0; /* no more data left to deliver */
}

int SMTPSender::send(std::string email, std::string to, std::string from)
{
    CURL *curl;
    struct curl_slist *recipients = NULL;
    struct WriteThis wt;

    wt.readptr = email.c_str();
    wt.sizeleft = email.length();

    // "from" is for the envelope reverse-path
    // "to" becomes the envelope forward-path */

    curl = curl_easy_init();
    if (curl)
    {
        /* This is the URL for your mailserver */
        m_curl_status = curl_easy_setopt(curl, CURLOPT_URL, m_smtpurl.c_str());
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* Note that this option isn't strictly required, omitting it will result in
         * libcurl sending the MAIL FROM command with no sender data. All
         * autoresponses should have an empty reverse-path, and should be directed
         * to the address in the reverse-path which triggered them. Otherwise, they
         * could cause an endless loop. See RFC 5321 Section 4.5.5 for more details.
         */
        m_curl_status = curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* Note that the CURLOPT_MAIL_RCPT takes a list, not a char array.  */
        recipients = curl_slist_append(recipients, to.c_str());
        m_curl_status = curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* You provide the payload (headers and the body of the message) as the
         * "data" element. There are two choices, either:
         * - provide a callback function and specify the function name using the
         * CURLOPT_READFUNCTION option; or
         * - just provide a FILE pointer that can be used to read the data from.
         * The easiest case is just to read from standard input, (which is available
         * as a FILE pointer) as shown here.
         */

        /* we want to use our own read function */
        m_curl_status = curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* pointer to pass to our read function */
        m_curl_status = curl_easy_setopt(curl, CURLOPT_READDATA, &wt);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        m_curl_status = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;
	
        /* send the message (including headers) */
        m_curl_status = curl_easy_perform(curl);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_perform() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* free the list of recipients */
        curl_slist_free_all(recipients);

        /* curl won't send the QUIT command until you call cleanup, so you should be
         * able to re-use this connection for additional messages (setting
         * CURLOPT_MAIL_FROM and CURLOPT_MAIL_RCPT as required, and calling
         * curl_easy_perform() again. It may not be a good idea to keep the
         * connection open for a very long time though (more than a few minutes may
         * result in the server timing out the connection), and you do want to clean
         * up in the end.
         */
        curl_easy_cleanup(curl);
    }
    return 0;
}

int SMTPSender::sendFile(std::string fname, std::string to, std::string from)
{
    CURL *curl;
    struct curl_slist *recipients = NULL;
    FILE *f = fopen(fname.c_str(), "r");

    // "from" is for the envelope reverse-path
    // "to" becomes the envelope forward-path */

    curl = curl_easy_init();
    if (curl)
    {
        /* This is the URL for your mailserver */
        m_curl_status = curl_easy_setopt(curl, CURLOPT_URL, m_smtpurl.c_str());
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* Note that this option isn't strictly required, omitting it will result in
         * libcurl sending the MAIL FROM command with no sender data. All
         * autoresponses should have an empty reverse-path, and should be directed
         * to the address in the reverse-path which triggered them. Otherwise, they
         * could cause an endless loop. See RFC 5321 Section 4.5.5 for more details.
         */
        m_curl_status = curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* Note that the CURLOPT_MAIL_RCPT takes a list, not a char array.  */
        recipients = curl_slist_append(recipients, to.c_str());
        m_curl_status = curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* You provide the payload (headers and the body of the message) as the
         * "data" element. There are two choices, either:
         * - provide a callback function and specify the function name using the
         * CURLOPT_READFUNCTION option; or
         * - just provide a FILE pointer that can be used to read the data from.
         * The easiest case is just to read from standard input, (which is available
         * as a FILE pointer) as shown here.
         */
        m_curl_status = curl_easy_setopt(curl, CURLOPT_READDATA, f);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        m_curl_status = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_setopt() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* send the message (including headers) */
        m_curl_status = curl_easy_perform(curl);
        if (m_curl_status != CURLE_OK)
            cout << "curl_easy_perform() failed: " << curl_easy_strerror(m_curl_status) << endl;

        /* free the list of recipients */
        curl_slist_free_all(recipients);

        /* curl won't send the QUIT command until you call cleanup, so you should be
         * able to re-use this connection for additional messages (setting
         * CURLOPT_MAIL_FROM and CURLOPT_MAIL_RCPT as required, and calling
         * curl_easy_perform() again. It may not be a good idea to keep the
         * connection open for a very long time though (more than a few minutes may
         * result in the server timing out the connection), and you do want to clean
         * up in the end.
         */
        curl_easy_cleanup(curl);
    }
    return 0;
}

