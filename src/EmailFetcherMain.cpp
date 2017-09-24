/*
 * EmailFetcherMain.cpp
 *
 *  Created on: Aug 20, 2017
 *      Author: paulc
 */
#include <unistd.h>
#include <signal.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>

#include <iterator>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "S3Get.h"
#include "MaildirDownloader.h"
#include "MaildirFormatter.h"

#define DAYS_TO_CHECK 60

using namespace S3Downloader;
using namespace std;
using namespace Aws;
using namespace boost::property_tree;

using boost::property_tree::ptree;
using boost::property_tree::read_json;

struct config_item
{
    string name;
    string location;
    string topic_arn;
    string bucket;
    Downloader *pdownl;
    bool enabled;
};
typedef list<config_item> config_list;
int get_config(const string location, config_list &config);

// signal handler for TERM
bool quittin_time = false;
void term_sigaction(int signo, siginfo_t *sinfo, void *arg);

int main(int argc, char** argv)
{
    // setup the signal handler for TERM
    struct sigaction termaction;
    termaction.sa_sigaction = term_sigaction;
    termaction.sa_flags = SA_SIGINFO;
    sigemptyset(&termaction.sa_mask);
    sigaction(SIGTERM, &termaction, 0);
    sigaction(SIGINT, &termaction, 0);

    SDKOptions options;
    InitAPI(options);
    {
        list<config_item> config;
        MaildirFormatter mfmt;

        get_config("./cfg/mail.json", config);
        for (auto &item : config)
        {
            if (item.enabled)
            {
                item.pdownl = new MaildirDownloader(item.location.c_str(), DAYS_TO_CHECK, item.topic_arn.c_str(), item.bucket.c_str(), mfmt);
                item.pdownl->start();
                cout << "Email " << item.name << " is started" << endl;
            }
            else
                cout << "Email " << item.name << " is disabled" << endl;
        }

        while (!quittin_time)
            sleep(2);

        while (!config.empty())
        {
            config_item &item = config.back();
            if (item.enabled)
            {
               item.pdownl->stop();
               cout << item.name << " stopped" << endl;
               if (item.enabled)
                   delete item.pdownl;
            }
            config.pop_back();
        }
    }

    ShutdownAPI(options);
    return 0;
}

void term_sigaction(int signo, siginfo_t *, void *)
{
    quittin_time = true;
}

int get_config(const string location, config_list &config)
{
    string jstr;
    ptree pt;

    ifstream infile(location, ifstream::in);
    streamsize n;
    char buf[512];
    do
    {
        n = infile.readsome(buf, 511);
        buf[n] = '\0';
        jstr += buf;
    } while (infile.good() && n);
    infile.close();

    try
    {
        istringstream inp(jstr);
        read_json(inp, pt);
        boost::property_tree::ptree &mailboxes = pt.get_child("mailbox");
        for (boost::property_tree::ptree::iterator it = mailboxes.begin(); it != mailboxes.end(); it++)
        {
            config_item *pitem = new config_item;
            pitem->name = it->second.get<std::string> ("name");
            pitem->bucket = it->second.get<std::string> ("bucket");
            pitem->topic_arn = it->second.get<std::string> ("topic_arn");
            pitem->location = it->second.get<std::string> ("location");
            pitem->enabled = it->second.get<bool> ("enabled");
            config.push_back(*pitem);
        }
    } catch (const boost::property_tree::ptree_error &e)
    {
        std::cerr << "property_tree error = " << e.what() << std::endl;
        return -2;
    } catch (std::exception const& e)
    {
        std::cerr << "exception = " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

