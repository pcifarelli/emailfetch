/*
 * Downloader.h
 *
 *  Created on: Sep 9, 2017
 *      Author: Paul Cifarelli
 *
 *      The Downloader class is responsible for creating the SQS queue, subscribing to the SNS topic, and pulling the emails from S3 when notified.
 *
 *      A Downloader needs a Formatter to tell it how to open files (what to name them and such).
 *      The Downloader keeps track of what it has already downloaded so as to prevent duplicates.
 *
 *      It also "forgets" whether it downloaded a file after the number of days it was told at construction.  It does so by maintaining a map,
 *      which it reconstructs if restarted based on the names of the files in its working directory.  It must work with a Formatter, since
 *      Formatters know how to name files.
 *
 *      Note that it will always ignore "hidden" files (those that begin with a '.')
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

struct FileTracker
{
    std::string fname;
    std::time_t ftime;
};
typedef std::unordered_map<std::string, FileTracker> FileTrackerMap;

class Downloader
{
public:
    // NOTE: fmt must stay in scope
    Downloader(const int days, Aws::String topic_arn, Aws::String bucket_name, int verbose);
    Downloader(const int days, Aws::String topic_arn, Aws::String bucket_name, Formatter *fmt, int verbose);
    Downloader(const int days, Aws::String topic_arn, Aws::String bucket_name, FormatterList &fmtlist, int verbose);
    virtual ~Downloader();

    void addFormatter(Formatter *fmt);

    Aws::String &bucketName()
    {
        return m_bucket_name;
    }
    Aws::String &topicArn()
    {
        return m_topic_arn;
    }
    Aws::String &queueUrl()
    {
        return m_queue_url;
    }
    Aws::String &queueArn()
    {
        return m_queue_arn;
    }
    Aws::String &subscriptionArn()
    {
        return m_subscription_arn;
    }

    void start();
    void stop();

protected:
    struct Tracker
    {
        FileTrackerMap *filemap;
        Formatter *fmt;
    };
    typedef std::list<Tracker *> TrackerList;

    // save only the objects that are not already in the directory given by dir
    // NOTE: only the filename is of interest, the contents are not.  Therefore the content of the file can be empty
    // (this can be used to keep track of the objects that you have streamed elsewhere)
    // "days" - number of days back to check
    virtual void saveNewObjects();
    virtual void saveNewObjects(Tracker &);

private:
    void init();
    FileTrackerMap *mkdirmap(Formatter &fmt, time_t secs_back);
    void purgeMap();                                        // purge m_days back
    void purgeMap(Tracker &trkr);                           // purge mback
    void purgeMap(std::time_t secs_back);                  // purge secs back
    void purgeMap(Tracker &trkr, std::time_t secs_back);   // purge secs back
    void printMap(Tracker &);

    void create_sqs_queue(Aws::String queue_name);
    void get_queue_url();
    void get_queue_arn();
    void delete_sqs_queue();
    void add_permission();
    void subscribe_topic();
    void unsubscribe_topic();
    void wait_for_message();

    static void *run(void *thisObj);
    pthread_t m_pthread_id;
    bool m_exit_thread;
    int m_exit_status;

    bool m_verbose;

    TrackerList m_trackerlist;
    Aws::String m_bucket_name;
    int m_days;

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
