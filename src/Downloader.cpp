/*
 * Downloader.cpp
 *
 *  Created on: Sep 9, 2017
 *      Author: paulc
 */

#include <dirent.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/CreateQueueResult.h>
#include <aws/sqs/model/GetQueueUrlRequest.h>
#include <aws/sqs/model/GetQueueUrlResult.h>
#include <aws/sqs/model/GetQueueAttributesRequest.h>
#include <aws/sqs/model/SetQueueAttributesRequest.h>
#include <aws/sqs/model/QueueAttributeName.h>
#include <aws/sns/model/SubscribeRequest.h>
#include <aws/sns/model/UnsubscribeRequest.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/ReceiveMessageResult.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/core/client/DefaultRetryStrategy.h>
#include <aws/sqs/model/DeleteQueueRequest.h>

#include "Downloader.h"

Downloader::Downloader(const Aws::String dir, const int days, Aws::String topic_arn, Aws::String bucket_name, Formatter &fmt) :
    m_dir(dir), m_days(days), m_topic_arn(topic_arn), m_bucket_name(bucket_name), m_fmt(fmt)
{
    mkdirmap(dir.c_str(), days * SECONDS_PER_DAY);

    // disable retries so that bad urls don't hang the exe via retry loop
    Aws::Client::ClientConfiguration client_cfg;
    client_cfg.retryStrategy = Aws::MakeShared<Aws::Client::DefaultRetryStrategy>("sqs_delete_queue", 0);
    client_cfg.requestTimeoutMs = SQS_REQUEST_TIMEOUT_MS;
    m_sqs = new Aws::SQS::SQSClient(client_cfg);

    Aws::String hostn;
    char host[NI_MAXHOST];
    if (!gethostname(host, sizeof(host)))
        hostn = host;
    else
        hostn = "dummy";

    pid_t pid = getpid();
    std::ostringstream out;
    out << m_bucket_name << "-" << hostn << "-" << pid;
    create_sqs_queue(out.str().c_str());
    get_queue_url();
    get_queue_arn();
    add_permission();
    subscribe_topic();
}

Downloader::~Downloader()
{
    unsubscribe_topic();
    delete_sqs_queue();
    delete m_sqs;
}

void Downloader::saveNewObjects()
{
    S3Get s3accessor(m_bucket_name);
    s3object_list list = s3accessor.getObjectList();
    std::time_t time_limit = m_days * SECONDS_PER_DAY;
    auto n = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(n);
    std::string dirname = m_dir.c_str();
    std::string c;

    std::size_t nn = m_dir.find_last_of('/');
    if (nn != std::string::npos && nn != m_dir.length() - 1)
        c = "/";

    dirname += c;

    if (!list.empty())
    {
        for (auto const &s3_object : list)
        {
            std::chrono::system_clock::time_point t = s3_object.GetLastModified().UnderlyingTimestamp();
            std::time_t ts = std::chrono::system_clock::to_time_t(t);

            if (now - ts < time_limit)
            {
                std::string fname = m_fmt.mkname(s3_object);
                Aws::String key = m_fmt.getKey(fname.c_str());
                std::string input = key.c_str();
                std::unordered_map<std::string, FileTracker>::const_iterator got = m_filemap.find(input);

                if (got == m_filemap.end()) // not found in map
                {
                    //std::cout << "saving object from s3" << std::endl;
                    s3accessor.objSaveAs(s3_object, m_dir, m_fmt);
                    std::string fullpath = dirname + fname;
                    FileTracker ft =
                    { fullpath, ts };

                    std::pair<std::string, FileTracker> hashent(key.c_str(), ft);
                    m_filemap.insert(hashent);
                }
            }
        }
    }
}

int Downloader::mkdirmap(std::string dirname, time_t secs_back)
{
    DIR *dir;
    struct dirent *ent;
    struct dirent entry;
    struct stat fstats;
    std::string fname;
    std::string fullpath;
    std::string c;

    std::size_t n = dirname.find_last_of('/');
    if (n != std::string::npos && n != dirname.length() - 1)
        c = "/";

    if ((dir = opendir(dirname.c_str())) != NULL)
    {
        do
        {
            readdir_r(dir, &entry, &ent);
            if (ent)
            {
                fname = ent->d_name;
                fullpath = dirname + c + fname;
                if (!stat(fullpath.c_str(), &fstats))
                    if (S_ISREG(fstats.st_mode))
                    {
                        auto n = std::chrono::system_clock::now();
                        std::time_t now = std::chrono::system_clock::to_time_t(n);

                        if (now - fstats.st_mtim.tv_sec < secs_back)
                        {
                            Aws::String name = fname.c_str();
                            Aws::String key = m_fmt.getKey(name);
                            FileTracker ft =
                            { fullpath, fstats.st_mtim.tv_sec };

                            std::pair<std::string, FileTracker> hashent(key.c_str(), ft);
                            m_filemap.insert(hashent);
                        }
                    }
            }
        } while (ent);
        closedir(dir);
    }
    else
    {
        /* could not open directory */
        return -1;
    }

    return 0;
}

void Downloader::purgeMap(std::time_t time_limit)
{
    auto n = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(n);
    std::vector<std::string> to_remove;

    for (auto it = m_filemap.begin(); it != m_filemap.end(); ++it)
    {
        if (now - it->second.ftime > time_limit)
            to_remove.push_back(it->first);
    }
    for (auto it = to_remove.begin(); it != to_remove.end(); ++it)
        m_filemap.erase(*it);
}

void Downloader::purgeMap()
{
    std::time_t time_limit = m_days * SECONDS_PER_DAY;
    purgeMap(time_limit);
}

void Downloader::printMap()
{
    std::time_t time_limit = m_days * SECONDS_PER_DAY;
    auto n = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(n);

    for (auto it = m_filemap.begin(); it != m_filemap.end(); ++it)
    {
        std::cout << " " << it->first << ":" << it->second.ftime << std::endl;
    }
}

void Downloader::create_sqs_queue(Aws::String queue_name)
{
    Aws::SQS::Model::CreateQueueRequest cq_req;
    cq_req.SetQueueName(queue_name);

    m_queue_name = queue_name;

    auto cq_out = m_sqs->CreateQueue(cq_req);
    if (cq_out.IsSuccess())
    {
        std::cout << "Successfully created queue " << m_queue_name << std::endl;
    }
    else
    {
        std::cout << "Error creating queue " << m_queue_name << ": " << cq_out.GetError().GetMessage() << std::endl;
    }
}

void Downloader::get_queue_url()
{
    Aws::SQS::Model::GetQueueUrlRequest gqu_req;
    gqu_req.SetQueueName(m_queue_name);

    auto gqu_out = m_sqs->GetQueueUrl(gqu_req);
    if (gqu_out.IsSuccess())
    {
        m_queue_url = gqu_out.GetResult().GetQueueUrl();
        std::cout << "Queue " << m_queue_name << " has url " << m_queue_url << std::endl;
    }
    else
    {
        std::cout << "Error getting url for queue " << m_queue_name << ": " << gqu_out.GetError().GetMessage() << std::endl;
    }
}

void Downloader::get_queue_arn()
{
    Aws::SQS::Model::GetQueueAttributesRequest gqa_req;
    gqa_req.SetQueueUrl(m_queue_url);
    gqa_req.AddAttributeNames(Aws::SQS::Model::QueueAttributeName::QueueArn);

    auto gqa_out = m_sqs->GetQueueAttributes(gqa_req);
    if (gqa_out.IsSuccess())
    {
        auto attr = gqa_out.GetResult().GetAttributes();
        auto it = attr.begin();
        m_queue_arn = it->second;
        std::cout << "Queue " << m_queue_name << " has arn " << m_queue_arn << std::endl;
    }
    else
    {
        std::cout << "Error getting url for queue " << m_queue_name << ": " << gqa_out.GetError().GetMessage() << std::endl;
    }
}

void Downloader::delete_sqs_queue()
{
    Aws::SQS::Model::DeleteQueueRequest dq_req;
    dq_req.SetQueueUrl(m_queue_url);

    auto dq_out = m_sqs->DeleteQueue(dq_req);
    if (dq_out.IsSuccess())
    {
        std::cout << "Successfully deleted queue with url " << m_queue_url << std::endl;
    }
    else
    {
        std::cout << "Error deleting queue " << m_queue_url << ": " << dq_out.GetError().GetMessage() << std::endl;
    }
}

void Downloader::add_permission()
{
    Aws::SQS::Model::SetQueueAttributesRequest apr;
    Aws::String perm_policy =
        "{ \
        \"Version\":\"2012-10-17\", \
        \"Statement\":[ \
            { \
                \"Sid\":\"Policy-"
            + m_topic_arn
            + "\", \
                \"Effect\":\"Allow\", \
                \"Principal\":\"*\", \
                \"Action\":\"sqs:SendMessage\", \
                \"Resource\":\""
            + m_queue_arn
            + "\", \
                \"Condition\":{ \
                    \"ArnEquals\":{ \
                        \"aws:SourceArn\":\""
            + m_topic_arn
            + "\" \
                                } \
                            } \
                        } \
                    ] \
                }";

    apr.SetQueueUrl(m_queue_url);
    apr.AddAttributes(Aws::SQS::Model::QueueAttributeName::Policy, perm_policy);

    auto out = m_sqs->SetQueueAttributes(apr);
    if (!out.IsSuccess())
    {
        std::cout << "Error getting url for queue " << m_queue_name << ": " << out.GetError().GetMessage() << std::endl;
    }
    else
        std::cout << "successfully added policy allowing sns topic to send messages" << std::endl;
}

void Downloader::subscribe_topic()
{
    Aws::SNS::Model::SubscribeRequest sr;

    sr.SetProtocol("sqs");
    sr.SetEndpoint(m_queue_arn);
    sr.SetTopicArn(m_topic_arn);
    auto out = m_sns.Subscribe(sr);

    if (out.IsSuccess())
    {
        m_subscription_arn = out.GetResult().GetSubscriptionArn();
        std::cout << "successfully subscribed queue " << m_queue_arn << " to topic " << m_topic_arn << " with subscription arn "
            << m_subscription_arn << std::endl;
    }
    else
    {
        std::cout << "Error subscribing queue " << m_queue_arn << " to topic " << m_topic_arn << " :" << out.GetError().GetMessage()
            << std::endl;
    }

}

void Downloader::unsubscribe_topic()
{
    Aws::SNS::Model::UnsubscribeRequest ur;

    ur.SetSubscriptionArn(m_subscription_arn);
    auto out = m_sns.Unsubscribe(ur);

    if (out.IsSuccess())
    {
        std::cout << "successfully unsubscribed queue " << m_queue_arn << " from topic " << m_topic_arn << std::endl;
    }
    else
    {
        std::cout << "Error subscribing queue " << m_queue_arn << " to topic " << m_topic_arn << " :" << out.GetError().GetMessage()
            << std::endl;
    }

}

void Downloader::wait_for_message()
{
    Aws::SQS::Model::ReceiveMessageRequest rm_req;
    rm_req.SetQueueUrl(m_queue_url);
    rm_req.SetMaxNumberOfMessages(1);
    rm_req.SetWaitTimeSeconds(SQS_WAIT_TIME); // wait this many seconds if there are no messages before returning

    auto rm_out = m_sqs->ReceiveMessage(rm_req);
    if (!rm_out.IsSuccess())
    {
        std::cout << "Error receiving message from queue " << m_queue_url << ": " << rm_out.GetError().GetMessage() << std::endl;
        return;
    }

    const auto& messages = rm_out.GetResult().GetMessages();
    if (messages.size() == 0)
    {
        //std::cout << "No messages received from queue " << m_queue_url << std::endl;
        return;
    }

    const auto& message = messages[0];
    //std::cout << "Received message:" << std::endl;
    //std::cout << "  MessageId: " << message.GetMessageId() << std::endl;
    //std::cout << "  ReceiptHandle: " << message.GetReceiptHandle() << std::endl;
    //std::cout << "  Body: " << message.GetBody() << std::endl << std::endl;

    Aws::SQS::Model::DeleteMessageRequest dm_req;
    dm_req.SetQueueUrl(m_queue_url);
    dm_req.SetReceiptHandle(message.GetReceiptHandle());

    auto dm_out = m_sqs->DeleteMessage(dm_req);
    if (dm_out.IsSuccess())
    {
        //std::cout << "Successfully deleted message " << message.GetMessageId() << " from queue " << m_queue_url << std::endl;
    }
    else
    {
        std::cout << "Error deleting message " << message.GetMessageId() << " from queue " << m_queue_url << ": "
            << dm_out.GetError().GetMessage() << std::endl;
    }
}

void Downloader::start()
{
    m_exit_thread = false;
    m_exit_status = 0;
    if (pthread_create(&m_pthread_id, NULL, Downloader::run, (void *) this))
        std::cout << "Unable to start Downloader for " << bucketName() << std::endl;
    else
        std::cout << "Started downloader for " << bucketName() << std::endl;
}

void Downloader::stop()
{
    m_exit_thread = true;
    int *retval;
    std::cout << "Stopped downloader for " << bucketName() << std::endl;
    pthread_join(m_pthread_id, (void **)&retval);
}

void *Downloader::run(void *arg)
{
    Downloader *thisObj = (Downloader *) arg;
    while (1)
    {
        thisObj->wait_for_message();
        thisObj->saveNewObjects();

        if (thisObj->m_exit_thread)
            pthread_exit(&thisObj->m_exit_status);
    }
}
