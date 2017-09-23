/*
 * EmailFetcherMain.cpp
 *
 *  Created on: Aug 20, 2017
 *      Author: paulc
 */
#include <unistd.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>

#include <iterator>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::read_json;

#include "S3Get.h"
#include "Downloader.h"
#include "MaildirFormatter.h"

#define DAYS_TO_CHECK 60

using namespace S3Downloader;

struct config_item
{
    std::string name;
    std::string location;
    std::string topic_arn;
    std::string bucket;
    Downloader *pdownl;
    bool enabled;
};
typedef std::list<config_item> config_list;
int get_config(const std::string location, config_list &config);

int main(int argc, char** argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        std::list<config_item> config;
        MaildirFormatter mfmt;

        get_config("./cfg/mail.json", config);
        for (auto &item : config)
        {
            if (item.enabled)
            {
                item.pdownl = new Downloader(item.location.c_str(), DAYS_TO_CHECK, item.topic_arn.c_str(), item.bucket.c_str(), mfmt);
                item.pdownl->start();
                std::cout << "Email " << item.name << " is started" << std::endl;
            }
            else
                std::cout << "Email " << item.name << " is disabled" << std::endl;
        }

        sleep(6);

        while (!config.empty())
        {
            config_item &item = config.back();
            std::cout << item.name << " stopped" << std::endl;
            if (item.enabled)
            {
               item.pdownl->stop();
               if (item.enabled)
                   delete item.pdownl;
            }
            config.pop_back();
        }
    }

    Aws::ShutdownAPI(options);
    return 0;
}

int get_config(const std::string location, config_list &config)
{
    std::string jstr;
    ptree pt;

    std::ifstream infile(location, std::ifstream::in);
    std::streamsize n;
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
        std::istringstream inp(jstr);
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
