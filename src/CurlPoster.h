/*
 * CurlPoster.h
 *
 *  Created on: Oct 28, 2017
 *      Author: paulc
 */

#ifndef SRC_CURLPOSTER_H_
#define SRC_CURLPOSTER_H_

#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <iostream>

namespace S3Downloader
{

class CurlPoster
{
public:
    CurlPoster(std::string url);
    virtual ~CurlPoster();
    void post(std::string jstr);
    std::string getResult() const { return m_result; }

private:
    struct WriteThis {
      const char *readptr;
      int sizeleft;
    };

    std::string m_url;
    std::string m_result;

    static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp);
    static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
    int postIt(const char *url, const char *data, int sz);
};

} /* namespace S3Downloader */

#endif /* SRC_CURLPOSTER_H_ */
