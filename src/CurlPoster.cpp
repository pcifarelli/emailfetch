/*
 * CurlPoster.cpp
 *
 *  Created on: Oct 28, 2017
 *      Author: paulc
 */

#include "CurlPoster.h"

namespace S3Downloader
{

CurlPoster::CurlPoster( std::string url ) :
        m_url(url)
{
    // TODO Auto-generated constructor stub

}

CurlPoster::~CurlPoster()
{
    // TODO Auto-generated destructor stub
}

size_t CurlPoster::read_callback( void *dest, size_t size, size_t nmemb, void *userp )
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

size_t CurlPoster::write_data( void *buffer, size_t size, size_t nmemb, void *userp )
{
    CurlPoster *thisObj = (CurlPoster *) userp;

    thisObj->m_result.assign((char *) buffer, size * nmemb);
    return size * nmemb;
}

void CurlPoster::post( std::string jstr )
{
    postIt(m_url.c_str(), jstr.c_str(), jstr.length());
}

int CurlPoster::postIt( const char *url, const char *data, int sz )
{
    CURL *curl;
    CURLcode res;

    struct WriteThis wt;

    wt.readptr = data;
    wt.sizeleft = sz;

    /* In windows, this will init the winsock stuff */
    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    /* Check for errors */
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(res));
        return 1;
    }

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl)
    {
        /* First set the URL that is about to receive our POST. */
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* Now specify we want to POST data */
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

        /* pointer to pass to our read function */
        curl_easy_setopt(curl, CURLOPT_READDATA, &wt);

        /* capture the result in our own write function */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

        /* pointer to pass to the write function */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

        /* get verbose debug output please */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); //0 disable messages

        /*
         If you use POST to a HTTP 1.1 server, you can send data without knowing
         the size before starting the POST if you use chunked encoding. You
         enable this by adding a header like "Transfer-Encoding: chunked" with
         CURLOPT_HTTPHEADER. With HTTP 1.0 or without chunked transfer, you must
         specify the size in the request.
         */
        struct curl_slist *opts = NULL;

        opts = curl_slist_append(opts, "Transfer-Encoding: chunked");
        opts = curl_slist_append(opts, "Expect:");
        opts = curl_slist_append(opts, "Content-Type: application/json");
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, opts);
        /* use curl_slist_free_all() after the *perform() call to free this
         list again */

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_slist_free_all(opts);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}

} /* namespace S3Downloader */
