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

#include <aws/core/utils/json/JsonSerializer.h>

const char *default_config_file = "./cfg/emailfetch.json";
struct program_defaults
{
    uid_t euser;
    gid_t egroup;
} defaults =
{ 5000, 5000 };

enum workflow_type
{
    NONSLOT,   // send to fast email
    SLOT,      // post to UCDP
    BOTH       // sending to fast email and posting to UCDP
};

struct location_type
{
    workflow_type type;
    string        destination;
};
typedef list<location_type> location_list;

struct config_item
{
    string name;                     // the name of this item (anything you want, for logging)
    location_list locations;         // an array of locations to send to
    string topic_arn;                // the arn of the topic to wait on
    string bucket;                   // the S3 bucket to download from
    string workdir;                  // the working directory to save files into (only required for SLOT-only workflow, although can be specified for any)
    Downloader *pdownl;              // the downloader object that does the downloading
    bool has_nonslot_workflow;       // are any of the locations email workflow destinations? (determines which Downloader class to use)
    bool enabled;                    // is this mailbox enabled in the configuration
};

typedef list<config_item> config_list;
int get_config(const string location, program_defaults &defaults, config_list &config);

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
    for (auto &item : mailboxconfig)
    {
        cout << "CONFIG: " << item.name;
        cout << (item.enabled ? " is enabled" : " is disabled");
        cout << (item.has_nonslot_workflow ? " and has non-slot workflow\n" : "\n");
        cout << "CONFIG: " << "S3 Bucket:     " << item.bucket << endl;
        cout << "CONFIG: " << "SNS Topic ARN: " << item.topic_arn << endl;
        cout << "CONFIG: " << "Locations:" << endl;
        for (auto &loc : item.locations)
            cout << "CONFIG: " << "   " << loc.destination << endl;
    }

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

        for (auto &item : mailboxconfig)
        {
            if (item.enabled)
            {
                FormatterList fmt_list;
                Formatter *mfmt;          // a "formatter" is responsible for providing services to stream and save the file

                for (auto &loc : item.locations)
                {
                    switch (loc.type)
                    {
                        case SLOT:
                            mfmt = new UCDPFormatter();
                            fmt_list.push_back(*mfmt);
                            break;
                        case NONSLOT:
                            mfmt = new MaildirFormatter();
                            fmt_list.push_back(*mfmt);
                            break;
                        case BOTH:
                            cout << "ERROR: BOTH workflow not yet implemented" << endl;
                            break;
                        default:
                            cout << "ERROR: Unknown workflow encountered" << endl;
                    }
                }
                // instantiating a Downloader results in a new thread that waits on the notification
                item.pdownl = new MaildirDownloader(item.locations.front().destination.c_str(), DAYS_TO_CHECK,
                    item.topic_arn.c_str(), item.bucket.c_str(), mfmt);
                // start the thread
                item.pdownl->start();
                cout << "Email " << item.name << " is started" << endl;
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

void get_program_defaults(Utils::Json::JsonValue &jv, program_defaults &defaults)
{
    Utils::Json::JsonValue jdefaults = jv.GetObject("defaults");
    // who knows, maybe we'll need this to be thread safe
    int sz = 0, ret = 0;
    if (jdefaults.ValueExists("user"))
    {
        string user = jdefaults.GetObject("user").AsString().c_str();
        if ((sz = sysconf(_SC_GETPW_R_SIZE_MAX)) > 0)
        {
            char buf[sz];
            struct passwd pwent, *ppent = &pwent;
            if ((ret = getpwnam_r(user.c_str(), &pwent, buf, sz, &ppent)) == 0 && ppent)
                defaults.euser = pwent.pw_uid;
            else
            {
                int lerrno = errno;
                char ebuf[1024], *p;
                if (lerrno && (p = strerror_r(lerrno, ebuf, 1024)))
                    cout << "CONFIG: Error: Failed to get uid for user " << user << " error (" << p << ")" << endl;
                else if (lerrno)
                    cout << "CONFIG: Error: Failed to get uid for user " << user << " errno (" << lerrno << ")" << endl;
                else
                    cout << "CONFIG: Error: Failed to get uid for user " << user << " (user not found)" << endl;
            }
        }
    }
    if (jdefaults.ValueExists("group"))
    {
        string grp = jdefaults.GetObject("group").AsString().c_str();
        if ((sz = sysconf(_SC_GETGR_R_SIZE_MAX)) > 0)
        {
            char buf[sz];
            struct group grent, *pgent;
            if ((ret = getgrnam_r(grp.c_str(), &grent, buf, sz, &pgent)) == 0 && pgent)
                defaults.egroup = grent.gr_gid;
            else
            {
                int lerrno = errno;
                char ebuf[1024], *p;
                if (lerrno && (p = strerror_r(lerrno, ebuf, 1024)))
                    cout << "CONFIG: Error: Failed to get gid for group " << grp << " error (" << p << ")" << endl;
                else if (lerrno)
                    cout << "CONFIG: Error: Failed to get gid for group " << grp << " errno (" << lerrno << ")" << endl;
                else
                    cout << "CONFIG: Error: Failed to get gid for group " << grp << " (group not found)" << endl;
            }
        }
    }
}

void get_mailbox_config(Utils::Json::JsonValue &jv, config_list &config)
{
    Utils::Array<Utils::Json::JsonValue> arr = jv.GetArray("mailbox");

    for (int i = 0; i < arr.GetLength(); i++)
    {
        config_item *pitem = new config_item;

        if (arr[i].ValueExists("bucket"))
            pitem->bucket = arr[i].GetString("bucket").c_str();
        else
        {
            cout << "CONFIG: Error: no bucket name specified for this downloader" << endl;
            continue;
        }

        if (arr[i].ValueExists("name"))
            pitem->name = arr[i].GetString("name").c_str();
        else
            pitem->name = pitem->bucket;

        if (arr[i].ValueExists("topic_arn"))
            pitem->topic_arn = arr[i].GetString("topic_arn").c_str();
        else
        {
            cout << "CONFIG: Error: no topic arn specified for downloader " << pitem->name << endl;
            continue;
        }
        pitem->workdir = "";
        if (arr[i].ValueExists("workdir"))
            pitem->workdir = arr[i].GetString("workdir").c_str();

        Utils::Array<Utils::Json::JsonValue> locations = arr[i].GetArray("locations");
        int nlocs = locations.GetLength();
        for (int j = 0; j < nlocs; j++)
        {
            location_type *ploc = new location_type;
            ploc->destination = locations[j].AsString().c_str();
            std::size_t n = ploc->destination.find_last_of('/');
            if (n != std::string::npos && n != ploc->destination.length() - 1)
                ploc->destination += "/";

            if (ploc->destination.substr(0, 8) == "https://" || ploc->destination.substr(0, 7) == "http://")
            {
                ploc->type = SLOT;
            }
            else
            {
                ploc->type = NONSLOT;
                if (pitem->workdir == "")
                    pitem->workdir = ploc->destination + "tmp";
                pitem->has_nonslot_workflow = true;
            }
            pitem->locations.push_back(*ploc);
        }

        if (pitem->workdir == "")
        {
            cout << "CONFIG: Error: Slot-only workflow defined for " << pitem->name << " without specifying a workdir" << endl;
            continue;
        }

        pitem->enabled = arr[i].GetBool("enabled");
        config.push_back(*pitem);
    }
}

int get_config(const string location, program_defaults &defaults, config_list &config)
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

    // program defaults
    get_program_defaults(jv, defaults);

    // mailbox config
    get_mailbox_config(jv, config);

    return 0;
}

