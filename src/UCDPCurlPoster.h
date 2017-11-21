/*
 * CurlPoster.h
 *
 *  Created on: Oct 28, 2017
 *      Author: paulc
 */

#ifndef SRC_UCDPCURLPOSTER_H_
#define SRC_UCDPCURLPOSTER_H_

#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <iostream>

namespace S3Downloader
{

class UCDPCurlPoster
{
public:
    UCDPCurlPoster(std::string url, bool chunked = false);
    UCDPCurlPoster(std::string hostname, unsigned short port, std::string ip, std::string certificate, std::string password, bool chunked = false,
        std::string trclientid = "eapfastemail", std::string trfeedid = "news_eapfe", std::string trmessagetype = "EmailML");
    virtual ~UCDPCurlPoster();

    void setProxy(std::string proxy);
    void setNoProxy()
    {
        setProxy("");
    }
    void setVerboseOutput();
    void setQuietOutput();

    // this is the workhorse
    void post(int messageprio, std::string trmessageid, std::string jstr);

    CURLcode status() const
    {
        return m_curl_status;
    }
    std::string getResult() const
    {
        return m_result;
    }

private:
    bool m_chunked;
    std::string m_trclientid;
    std::string m_trfeedid;
    std::string m_trmessagetype;
    int messageprio;

    struct WriteThis
    {
        const char *readptr;
        int sizeleft;
    };

    CURL *m_curl;
    CURLcode m_curl_status;
    struct curl_slist *m_opts;
    struct curl_slist *m_resolve;

    std::string m_url;
    std::string m_result;

    void init();
    void setHeaders(long messagelength, int messageprio, std::string trmessageid);
    void set_ServerNameIndication(std::string hostname, unsigned short port, std::string ip);
    void set_Certificate(std::string certificate, std::string password);
    static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp);
    static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
    int postIt(const char *url, const char *data, int sz);
};

} /* namespace S3Downloader */

#endif /* SRC_UCDPCURLPOSTER_H_ */
