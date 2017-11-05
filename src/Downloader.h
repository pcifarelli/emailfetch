/*
 * Downloader.h
 *
 *  Created on: Sep 9, 2017
 *      Author: paulc
 */

#ifndef SRC_DOWNLOADER_H_
#define SRC_DOWNLOADER_H_

#include <aws/sqs/SQSClient.h>
#include <aws/sns/SNSClient.h>
#include "S3Get.h"
#include <iostream>
#include <unordered_map>
#include <pthread.h>

namespace S3Downloader
{


#define SECONDS_PER_DAY        86400
#define SQS_REQUEST_TIMEOUT_MS 60000
#define SQS_WAIT_TIME          2


class Downloader
{
public:
    // NOTE: s3 and fmt must stay in scope
    Downloader(const int days, Aws::String topic_arn, Aws::String bucket_name, Formatter &fmt);
    virtual ~Downloader();

    Aws::String &bucketName() { return m_bucket_name; }

    void start();
    void stop();

protected:
    // save only the objects that are not already in the directory given by dir
    // NOTE: only the filename is of interest, the contents are not.  Therefore the content of the file can be empty
    // (this can be used to keep track of the objects that you have streamed elsewhere)
    // "days" - number of days back to check
    virtual void saveNewObjects();

private:
    int mkdirmap(std::string dirname, time_t secs_back);
    void purgeMap();                          // purge m_days back
    void purgeMap( std::time_t secs_back );   // purge secs_back
    void printMap();

    void create_sqs_queue(Aws::String queue_name);
    void get_queue_url();
    void get_queue_arn();
    void delete_sqs_queue();
    void add_permission();
    void subscribe_topic();
    void unsubscribe_topic();
    void wait_for_message();

    static void *run( void *thisObj );
    pthread_t m_pthread_id;
    bool m_exit_thread;
    int  m_exit_status;

    Formatter &m_fmt;
    Aws::String m_bucket_name;
    int m_days;

    struct FileTracker
    {
        std::string fname;
        std::time_t ftime;
    };
    std::unordered_map<std::string, FileTracker> m_filemap;

    Aws::String m_queue_name;
    Aws::String m_queue_url;
    Aws::String m_queue_arn;
    Aws::String m_topic_arn;
    Aws::String m_subscription_arn;
    Aws::SQS::SQSClient *m_sqs;
    Aws::SNS::SNSClient m_sns;

};

}
#endif /* SRC_DOWNLOADER_H_ */
