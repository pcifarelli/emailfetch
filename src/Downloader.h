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

struct DownloaderDestination
{
    Aws::String destination;
    Formatter &fmt;
};

typedef std::list<DownloaderDestination> DestinationList;

class Downloader
{
public:
    // NOTE: fmtlist must stay in scope
    Downloader(const DestinationList &dlist, const int days, Aws::String topic_arn, Aws::String bucket_name);
    virtual ~Downloader();

    Aws::String &bucketName()
    {
        return m_bucket_name;
    }

    void start();
    void stop();

protected:
    struct FileTracker
    {
        std::string fname;
        std::time_t ftime;
    };

    typedef std::unordered_map<std::string, FileTracker> FileMap;
    class Maildestination
    {
    public:
        Aws::String m_track_dir;
        Aws::String m_save_dir;
        Formatter  *m_fmt;
        FileMap    *m_filemap;
    };

    // save only the objects that are not already in the directory given by dir
    // NOTE: only the filename is of interest, the contents are not.  Therefore the content of the file can be empty
    // (this can be used to keep track of the objects that you have streamed elsewhere)
    // "days" - number of days back to check
    virtual void saveNewObjects();

    virtual void saveNewObjects(Maildestination &md);

    std::list<Maildestination *> m_mailbox_list;

private:
    int mkdirmap(Maildestination *md, time_t secs_back);
    void purgeMap(Maildestination &md);                          // purge m_days back
    void purgeMap(Maildestination &md, std::time_t secs_back);   // purge secs_back
    void printMap(Maildestination &md);

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

    DestinationList &m_destinations;

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
