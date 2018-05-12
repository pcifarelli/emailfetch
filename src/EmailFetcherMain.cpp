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
#include <CurlPoster.h>

#include <iterator>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "S3Get.h"
#include "MaildirFormatter.h"
#include "UCDPFormatter.h"
#include "EmailForwarderFormatter.h"
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
        << "         -m <mailbox file>\tmailbox config file" << endl
        << "         -vN\t\t\tverbose output level, N=1,2,3 or 4" << endl << "         -h\t\t\tthis help" << endl;
    cout << "NOTE: mailboxes can be defined in both the main config file and in a separate mailbox config file" << endl;
    exit(0);
}

int main(int argc, char** argv)
{
    const char *config_file = default_config_file;
    const char *mailbox_file = NULL;

    // read the command line options
    int option_char;
    while ((option_char = getopt(argc, argv, "f:m:v:h")) != -1)
        switch (option_char)
        {
            case 'f':
                config_file = optarg;
                break;
            case 'm':
                mailbox_file = optarg;
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
    {
        cout << "Using " << config_file << " for configuration" << endl;
        if (mailbox_file)
            cout << "Using " << mailbox_file << " for mailbox configuration" << endl;
    }

    // setup the AWS SDK
    SDKOptions options;
    InitAPI(options);
    {
        list<config_item> mailboxconfig; // this is our mailbox configuration
        // NOTE that this means we must have read permission in the account that the program is started in (we don't know what our euid/guid should be yet)
        if (mailbox_file)
            get_config_by_file(config_file, mailbox_file, defaults, mailboxconfig);
        else
            get_config_by_file(config_file, "", defaults, mailboxconfig);

        if (verbose)
            print_config(mailboxconfig);

        //*********************************************
        // shutdown the AWS SDK
        ShutdownAPI(options);
        return 0;
        //*********************************************

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

        S3Downloader::FormatterList fmtlist;

        for (auto &item : mailboxconfig)
        {
            if (item.enabled)
            {
                string email = item.name + "@" + item.domainname;
                bool relay_forwarding_configured = false;          // only 1 location per item should be configured with relay_forwarding

                // instantiating a Downloader and calling "start()" results in a new thread that waits on the notification
                item.pdownl = new Downloader(DAYS_TO_CHECK, item.topic_arn.c_str(), item.bucket.c_str(), verbose);
                for (auto &loc : item.locations)
                {
                    switch (loc.type)
                    {
                        case MAILBOX:
                            {
                                // a "Formatter" is responsible for providing services to stream and route the mail to a destination
                                // a "MaildirFormatter" saves emails in maildir format to be served via imap
                                MaildirFormatter *mailfmt;

                                Aws::String dir = loc.mailbox.destination.c_str();
                                if (item.enable_mxforwarding && !relay_forwarding_configured)
                                {
                                    mailfmt = new MaildirFormatter(dir, item.forward_servers, verbose);
                                    relay_forwarding_configured = true;
                                }
                                else
                                    mailfmt = new MaildirFormatter(dir, verbose);

                                item.pdownl->addFormatter(mailfmt);
                                if (verbose)
                                    cout << "Location " << loc.mailbox.destination << " of type MAILBOX added to " << item.name << endl;
                            }
                            break;
                        case UCDP:
                            {
                                // a "Formatter" is responsible for providing services to stream and route the mail to a destination
                                // a "UCDPFormatter" formats in TR.JSON and posts the email to UCDP
                                UCDPFormatter *ucdpfmt;

                                Aws::String workdir = loc.rest.workdir.c_str();

                                if (item.enable_mxforwarding && !relay_forwarding_configured)
                                {
                                    ucdpfmt = new UCDPFormatter(
                                        workdir,
                                        item.forward_servers,
                                        email,
                                        loc.rest.destination,
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
                                    relay_forwarding_configured = true;
                                }
                                else
                                    ucdpfmt = new UCDPFormatter(
                                        workdir,
                                        email,
                                        loc.rest.destination,
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
                                    cout << "Location " << loc.rest.destination << " of type UCDP added to " << item.name << endl;
                            }
                            break;
                        case FORWARD:
                            {
                                EmailForwarderFormatter *fwdfmt;
                                // a "Formatter" is responsible for providing services to stream and route the mail to a destination
                                // a "EmailForwarderFormatter" forwards a downloaded email to the list of destinations given by the email list
                                Aws::String dir = loc.forwarder.workdir.c_str();
                                outgoing_mail_server si = loc.forwarder.server_info;
                                if (item.enable_mxforwarding && !relay_forwarding_configured)
                                {
                                    fwdfmt = new EmailForwarderFormatter(loc.forwarder.from, loc.forwarder.email_destinations, si, dir, item.forward_servers, verbose);
                                    relay_forwarding_configured = true;
                                }
                                else
                                    fwdfmt = new EmailForwarderFormatter(loc.forwarder.from, loc.forwarder.email_destinations, si, dir, verbose);

                                item.pdownl->addFormatter(fwdfmt);
                                if (verbose)
                                {
                                    cout << "Location " << email << " with type FORWARDER added to " << item.name;
                                    if (verbose > 2)
                                    {
                                        cout << " with destinations:" << endl;
                                        for (auto &email : loc.forwarder.email_destinations)
                                            cout << "   " << email << endl;
                                    }
                                    else
                                        cout << endl;
                                }
                            }
                            break;

                        default:
                            cout << "Location type (" << loc.type << ") invalid for " << item.name << " - ignoring" << endl;
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

