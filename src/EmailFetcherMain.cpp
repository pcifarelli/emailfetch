/*
 * EmailFetcherMain.cpp
 *
 *  Created on: Aug 20, 2017
 *      Author: paulc
 */
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>

#include <iterator>

#include "S3Get.h"
#include "MaildirDownloader.h"
#include "MaildirFormatter.h"

#include "config.h"

#define DAYS_TO_CHECK 60

using namespace S3Downloader;
using namespace std;
using namespace Aws;

#include <aws/core/utils/json/JsonSerializer.h>

struct program_defaults
{
   uid_t euser;
   gid_t egroup;
} defaults = {
   5000,
   5000
};

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
    // try to set the effective userid and group id
    if (setegid(defaults.egroup))
        cout << "Unable to set effective gid to " << defaults.egroup << endl;

    if (seteuid(defaults.euser))
        cout << "Unable to set effective uid to " << defaults.euser << endl;

    // setup the signal handler for TERM
    struct sigaction termaction;
    termaction.sa_sigaction = term_sigaction;
    termaction.sa_flags = SA_SIGINFO;
    sigemptyset(&termaction.sa_mask);
    sigaction(SIGTERM, &termaction, 0);
    sigaction(SIGINT, &termaction, 0);
    sigaction(SIGQUIT, &termaction, 0);

    const char *default_config_file = "./cfg/emailfetch.json";
    const char *config_file = default_config_file;
    const char *default_mailboxconfig_file = "./cfg/mail.json";
    const char *mailboxconfig_file = default_mailboxconfig_file;

    // read the command line options
    int option_char;
    while ((option_char = getopt(argc, argv, "f:m:")) != -1)
      switch (option_char)
        {  
           case 'm': mailboxconfig_file = optarg; break;
           case 'f': config_file = optarg; break;
        }

    cout << "Using " << mailboxconfig_file << " for mailbox configuration" << endl;

    // setup the AWS SDK
    SDKOptions options;
    InitAPI(options);
    {
        list<config_item> config; // this is our configuration
        MaildirFormatter mfmt;    // a "formatter" is responsible for providing services to stream and save the file

        get_config(mailboxconfig_file, config);
        for (auto &item : config)
        {
            if (item.enabled)
            {
                // instantiating a Downloader results in a new thread that waits on the notification
                item.pdownl = new MaildirDownloader(item.location.c_str(), DAYS_TO_CHECK, item.topic_arn.c_str(), item.bucket.c_str(), mfmt);
                // start the thread
                item.pdownl->start();
                cout << "Email " << item.name << " is started" << endl;
            }
            else
                cout << "Email " << item.name << " is disabled" << endl;
        }

        // wait until signaled to quit
        while (!quittin_time)
            sleep(2);

        // cleanup the threads (wait for them)
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

    // shutdown the AWS SDK
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

    istringstream inp(jstr);
    Utils::Json::JsonValue jv(inp);

    Utils::Array<Utils::Json::JsonValue> arr = jv.GetArray("mailbox");

    for (int i = 0; i < arr.GetLength(); i++)
    {
        config_item *pitem = new config_item;

        pitem->name = arr[i].GetString("name").c_str();
        pitem->bucket = arr[i].GetString("bucket").c_str();
        pitem->topic_arn = arr[i].GetString("topic_arn").c_str();
        pitem->location = arr[i].GetString("location").c_str();
        pitem->enabled = arr[i].GetBool("enabled");

        config.push_back(*pitem);
    }

    return 0;
}

