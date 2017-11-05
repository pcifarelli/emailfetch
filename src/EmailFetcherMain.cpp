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
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "S3Get.h"
#include "MaildirDownloader.h"
#include "MaildirFormatter.h"
#include "UCDPFormatter.h"
#include "CurlPoster.h"

#include "config.h"

#define DAYS_TO_CHECK 60

using namespace S3Downloader;
using namespace std;
using namespace Aws;

const char *default_config_file = "./cfg/emailfetch.json";
#include "EmailFetcherConfig.h"
typedef list<Formatter>     FormatterList;

// signal handler for TERM
bool quittin_time = false;
void term_sigaction(int signo, siginfo_t *sinfo, void *arg);

int main(int argc, char** argv)
{
    const char *config_file = default_config_file;

    // read the command line options
    int option_char;
    while ((option_char = getopt(argc, argv, "f:m:")) != -1)
        switch (option_char)
        {
        case 'f':
            config_file = optarg;
            break;
        case 'h':
            cout << "Usage: emailfetch [-f <config file>]" << endl;
        }

    cout << "Using " << config_file << " for configuration" << endl;

    list<config_item> mailboxconfig; // this is our mailbox configuration
    // NOTE that this means we must have read permission in the account that the program is started in (we don't know what our euid/guid should be yet)
    get_config(config_file, defaults, mailboxconfig);
    print_config(mailboxconfig);

    // try to set the effective userid and group id
    if (setegid(defaults.egroup))
    {
        int lerrno = errno;
        char buf[1024], *p;
        if (lerrno && (p = strerror_r(lerrno, buf, 1024)))
            cout << "Unable to set effective gid to " << defaults.egroup << " (" << p << ")" << endl;
        else
            cout << "Unable to set effective gid to " << defaults.egroup << " errno (" << lerrno << ")" << endl;
    }

    if (seteuid(defaults.euser))
    {
        int lerrno = errno;
        char buf[1024], *p;
        if (lerrno && (p = strerror_r(lerrno, buf, 1024)))
            cout << "Unable to set effective uid to " << defaults.euser << " (" << p << ")" << endl;
        else
            cout << "Unable to set effective uid to " << defaults.euser << " error (" << lerrno << ")" << endl;
    }

    // setup the signal handler for TERM
    struct sigaction termaction;
    termaction.sa_sigaction = term_sigaction;
    termaction.sa_flags = SA_SIGINFO;
    sigemptyset(&termaction.sa_mask);
    sigaction(SIGTERM, &termaction, 0);
    sigaction(SIGINT, &termaction, 0);
    sigaction(SIGQUIT, &termaction, 0);

    // setup the AWS SDK
    SDKOptions options;
    InitAPI(options);
    {
        MaildirFormatter mailfmt;          // a "formatter" is responsible for providing services to stream and save the file

        for (auto &item : mailboxconfig)
        {
            if (item.enabled)
            {

                // instantiating a Downloader results in a new thread that waits on the notification
                for (auto &loc : item.locations)
                {
                    if (loc.type == NONSLOT)
                    {
                        item.pdownl = new MaildirDownloader(loc.mailbox.destination.c_str(), DAYS_TO_CHECK, item.topic_arn.c_str(), item.bucket.c_str(), mailfmt);

                        // start the thread
                        item.pdownl->start();
                        cout << "Email for " << item.name << ", location " << (loc.type==SLOT?loc.rest.url:loc.mailbox.destination) << " is started" << endl;
                    }
                }
            }
            else
                cout << "Email " << item.name << " is disabled" << endl;
        }

//        CurlPoster cp("http://10.0.1.103:8080/growler/rest/notify/My%20Laptop");
//        cp.post("{\"type\":\"SmartThings\", \
//                  \"title\":\"Secure\", \
//                  \"description\":\"this is a test\"}");
//        cout << "\nCompleted POST\n";
//        cout << cp.getResult();

        // wait until signaled to quit
        while (!quittin_time)
            sleep(2);

        // cleanup the threads (wait for them)
        while (!mailboxconfig.empty())
        {
            config_item &item = mailboxconfig.back();
            if (item.enabled)
            {
                item.pdownl->stop();
                cout << item.name << " stopped" << endl;
                if (item.enabled)
                    delete item.pdownl;
            }
            mailboxconfig.pop_back();
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


