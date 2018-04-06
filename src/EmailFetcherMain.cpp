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
#include "MaildirFormatter.h"
#include "UCDPFormatter.h"
#include "UCDPCurlPoster.h"

#include "config.h"

#define DAYS_TO_CHECK 60

using namespace S3Downloader;
using namespace std;
using namespace Aws;

const char *default_config_file = "./cfg/emailfetch.json";
#include "EmailFetcherConfig.h"

// verbose output
int verbose = 0;

// signal handler for TERM
bool quittin_time = false;
void term_sigaction(int signo, siginfo_t *sinfo, void *arg);

void usage()
{
    cout << "Usage: emailfetch [-f <config file>] [-vN] [-h]" << endl
        << "   where -f <config file>\tprovide alternative config file (default is ./cfg/emailfetch.cfg)" << endl
        << "         -vN\t\t\tverbose output level, N=1,2,3 or 4" << endl << "         -h\t\t\tthis help" << endl;
    exit(0);
}

int main(int argc, char** argv)
{
    const char *config_file = default_config_file;

    // read the command line options
    int option_char;
    while ((option_char = getopt(argc, argv, "f:m:v:h")) != -1)
        switch (option_char)
        {
            case 'f':
                config_file = optarg;
                break;
            case 'v':
                if (!strncmp(optarg, "1", 1))
                    verbose = 1;
                else if (!strncmp(optarg, "2", 1))
                    verbose = 2;
                else if (!strncmp(optarg, "3", 1))
                    verbose = 3;
                else if (!strncmp(optarg, "4", 1))
                    verbose = 4;
                else
                    usage();
                break;
            case 'h':
                default:
                usage();
        }

    if (verbose)
        cout << "Using " << config_file << " for configuration" << endl;

    list<config_item> mailboxconfig; // this is our mailbox configuration
    // NOTE that this means we must have read permission in the account that the program is started in (we don't know what our euid/guid should be yet)
    get_config(config_file, defaults, mailboxconfig);
    if (verbose)
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
            cout << "Unable to set effective uid to " << defaults.euser << " errno (" << lerrno << ")" << endl;
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
        S3Downloader::FormatterList fmtlist;

        for (auto &item : mailboxconfig)
        {
            if (item.enabled)
            {
                string email = item.name + "@" + item.domainname;

                // instantiating a Downloader and calling "start()" results in a new thread that waits on the notification
                item.pdownl = new Downloader(DAYS_TO_CHECK, item.topic_arn.c_str(), item.bucket.c_str(), verbose);
                for (auto &loc : item.locations)
                {
                    switch (loc.type)
                    {
                        case NONSLOT:
                            {
                                // a "Formatter" is responsible for providing services to stream and route the mail to a destination
                                // a "MaildirFormatter" saves emails in maildir format to be served via imap
                                MaildirFormatter *mailfmt;

                                Aws::String dir = loc.destination.c_str();
                                if (item.enable_forwarding)
                                    mailfmt = new MaildirFormatter(dir, item.forward_servers, verbose);
                                else
                                    mailfmt = new MaildirFormatter(dir, verbose);

                                item.pdownl->addFormatter(mailfmt);
                                if (verbose)
                                    cout << "Location " << loc.destination << " of type NONSLOT added to " << item.name << endl;
                            }
                            break;
                        case SLOT:
                            {
                                // a "Formatter" is responsible for providing services to stream and route the mail to a destination
                                // a "UCDPFormatter" formats in TR.JSON and posts the email to UCDP
                                UCDPFormatter *ucdpfmt;

                                Aws::String workdir = loc.rest.workdir.c_str();

                                if (!item.enable_forwarding)
                                    ucdpfmt = new UCDPFormatter(
                                        workdir,
                                        email,
                                        loc.destination,
                                        loc.rest.snihostname,
                                        loc.rest.port,
                                        loc.rest.certificate,
                                        loc.rest.certificatepassword,
                                        loc.rest.trclientid,
                                        loc.rest.trfeedid,
                                        loc.rest.trmessagetype,
                                        loc.rest.trmessageprio,
                                        loc.rest.validate_json,
                                        verbose);
                                else
                                    ucdpfmt = new UCDPFormatter(
                                        workdir,
                                        item.forward_servers,
                                        email,
                                        loc.destination,
                                        loc.rest.snihostname,
                                        loc.rest.port,
                                        loc.rest.certificate,
                                        loc.rest.certificatepassword,
                                        loc.rest.trclientid,
                                        loc.rest.trfeedid,
                                        loc.rest.trmessagetype,
                                        loc.rest.trmessageprio,
                                        loc.rest.validate_json,
                                        verbose);

                                item.pdownl->addFormatter(ucdpfmt);
                                if (verbose)
                                    cout << "Location " << loc.destination << " of type SLOT added to " << item.name << endl;
                            }
                            break;

                        default:
                            cout << "Location type invalid for " << item.name << " location " << loc.destination << " - ignoring" << endl;
                    }
                }
                // start the thread
                item.pdownl->start();
                if (verbose)
                    cout << "Email for " << item.name << " is started" << endl;
            }
            else if (verbose)
                cout << "Email " << item.name << " is disabled" << endl;
        }

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
                if (verbose)
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

