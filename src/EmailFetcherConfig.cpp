/*
 * EmailFetcherConfig.cpp
 *
 *  Created on: Nov 4, 2017
 *      Author: paulc
 */
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <iterator>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "EmailFetcherConfig.h"

#include "config.h"

using namespace std;
using namespace Aws;

struct program_defaults defaults =
{
    5000,
    5000,
    {
        "./mail/.%f",
        "./mail/%d/%u@%d/"
    },
    {
        15026,
        "./cfg/ucdp_eapfastemail.pem",
        "eapfastemail",
        "./mail/slot"
    }
};


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
    if (jdefaults.ValueExists("mailbox"))
    {
        Utils::Json::JsonValue mailboxdefaults = jdefaults.GetObject("mailbox");
        if (mailboxdefaults.ValueExists("publicdir"))
            defaults.mailbox_defaults.publicdir = mailboxdefaults.GetString("publicdir").c_str();
        if (mailboxdefaults.ValueExists("userdir"))
            defaults.mailbox_defaults.userdir = mailboxdefaults.GetString("userdir").c_str();
    }
    if (jdefaults.ValueExists("UCDP"))
    {
        Utils::Json::JsonValue ucdpdefaults = jdefaults.GetObject("UCDP");
        if (ucdpdefaults.ValueExists("port"))
            defaults.UCDP_defaults.port = ucdpdefaults.GetInteger("port");
        if (ucdpdefaults.ValueExists("snicertfile"))
            defaults.UCDP_defaults.snicertfile = ucdpdefaults.GetString("snicertfile").c_str();
        if (ucdpdefaults.ValueExists("clientname"))
            defaults.UCDP_defaults.clientname = ucdpdefaults.GetString("clientname").c_str();
        if (ucdpdefaults.ValueExists("workdir"))
            defaults.UCDP_defaults.workdir = ucdpdefaults.GetString("workdir").c_str();
    }
}

string replace_all(string str, const string &from, const string &to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

string create_location(string fmt, string &user, string &name, string &domainname)
{
    string result = fmt;

    result = replace_all( result, "%u", user);       // replace all "%u with the user's name
    result = replace_all( result, "%f", name);       // replace all "%f with the feed name
    result = replace_all( result, "%d", domainname); // replace all "%d with the domain name

    std::size_t n = result.find_last_of('/');
    if (n != std::string::npos && n != result.length() - 1)
        result += "/";

    return result;
}

string create_public_mailbox_location(program_defaults &defaults, string &name, string &domainname)
{
    return create_location(defaults.mailbox_defaults.publicdir, name, name, domainname);
}

string create_user_mailbox_location(program_defaults &defaults, string &user, string &name, string &domainname)
{
    return create_location(defaults.mailbox_defaults.userdir, user, name, domainname);
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
            cout << "CONFIG: Error: no AWS S3 bucket specified for this downloader -  ignoring" << endl;
            continue;
        }

        if (arr[i].ValueExists("email"))
        {
            string email = arr[i].GetString("email").c_str();
            istringstream e(email);
            getline(e, pitem->name, '@');
            getline(e, pitem->domainname, '@');
        }
        else
        {
            if (arr[i].ValueExists("name"))
                pitem->name = arr[i].GetString("name").c_str();
            else
            {
                cout << "CONFIG: Error: Cannot determine the email address for this downloader (name missing)" << endl;
                continue;
            }
            if (arr[i].ValueExists("domainname"))
                pitem->domainname = arr[i].GetString("domainname").c_str();
            else
            {
                cout << "CONFIG: Error: Cannot determine the email address for this downloader (domainname missing)" << endl;
                continue;
            }
        }

        if (arr[i].ValueExists("description"))
            pitem->description = arr[i].GetString("description").c_str();
        else
            pitem->description = pitem->name + "@" + pitem->domainname;

        if (arr[i].ValueExists("topic_arn"))
            pitem->topic_arn = arr[i].GetString("topic_arn").c_str();
        else
        {
            cout << "CONFIG: Error: no topic arn specified for downloader " << pitem->name << endl;
            continue;
        }

        Utils::Array<Utils::Json::JsonValue> locations = arr[i].GetArray("locations");
        int nlocs = locations.GetLength();
        for (int j = 0; j < nlocs; j++)
        {
            if (locations[j].ValueExists("mailbox"))
            {
                location_type *ploc = new location_type;
                bool is_mailbox = locations[j].GetBool("mailbox");
                if (is_mailbox)
                {
                    ploc->type = NONSLOT;
                    pitem->has_nonslot_workflow = true;
                    if (locations[j].ValueExists("public"))
                    {
                        bool is_public = locations[j].GetBool("public");
                        if (is_public)
                            ploc->mailbox.destination = create_public_mailbox_location(defaults, pitem->name, pitem->domainname);
                        else
                        {
                            if (locations[j].ValueExists("user"))
                            {
                                ploc->mailbox.user = locations[j].GetString("user").c_str();
                                ploc->mailbox.destination = create_user_mailbox_location(defaults, ploc->mailbox.user, pitem->name, pitem->domainname);
                            }
                            else
                            {
                                if (locations[j].ValueExists("workdir"))
                                {
                                    string fmt = locations[j].GetString("workdir").c_str();
                                    ploc->mailbox.user = "";
                                    ploc->mailbox.destination = create_location(fmt, ploc->mailbox.user, pitem->name, pitem->domainname);
                                }
                                else
                                {
                                    cout << "CONFIG: Error: Unable to find user or workdir element for mailbox location - ignoring this location" << endl;
                                    continue;
                                }
                            }
                        }
                    }
                    else
                    {
                        cout << "CONFIG: Error: required boolean element \"public\" not specified for mailbox - ignoring this location" << endl;
                        continue;
                    }
                }
                else // it goes to a URL
                {
                    ploc->type = SLOT;
                    // SNI is the default for UCDP, and UCDP is the default system to post to
                    if (locations[j].ValueExists("sni"))
                        ploc->rest.sni = locations[j].GetBool("sni");
                    else
                        ploc->rest.sni = true;

                    if (locations[j].ValueExists("workdir"))
                    {
                        string fmt = locations[j].GetString("workdir").c_str();
                        ploc->rest.workdir = create_location(fmt, ploc->mailbox.user, pitem->name, pitem->domainname);
                    }
                    else
                        ploc->rest.workdir = create_location(defaults.UCDP_defaults.workdir, ploc->mailbox.user, pitem->name, pitem->domainname);

                    if (locations[j].ValueExists("url"))
                    {
                        string fmt = locations[j].GetString("url").c_str();
                        string user = "";
                        ploc->rest.url = create_location(fmt, user, pitem->name, pitem->domainname);
                    }
                    else
                    {
                        cout << "CONFIG: Error: no URL specified for REST destination - ignoring this location" << endl;
                        continue;
                    }
                }

                pitem->locations.push_back(*ploc);
            }
            else
            {
                cout << "CONFIG: Error: required element \"mailbox\" boolean element not specified - ignoring this location" << endl;
            }
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

void print_config(config_list &mailboxconfig)
{
    for (auto &item : mailboxconfig)
    {
        cout << "CONFIG: " << item.description << endl;
        cout << "CONFIG: " << item.name << '@' << item.domainname;
        cout << (item.enabled ? " is enabled" : " is disabled");
        cout << (item.has_nonslot_workflow ? " and has non-slot workflow\n" : "\n");
        cout << "CONFIG: " << "S3 Bucket:     " << item.bucket << endl;
        cout << "CONFIG: " << "SNS Topic ARN: " << item.topic_arn << endl;
        cout << "CONFIG: " << "Locations:" << endl;
        for (auto &loc : item.locations)
            if (loc.type == SLOT)
            {
                cout << "CONFIG: Slot:     " << "   URL:         " << loc.rest.url << endl;
                cout << "CONFIG: Slot:     " << "   workdir:     " << loc.rest.workdir << endl;
                cout << "CONFIG: Slot:     " << "   " << (loc.rest.sni ? "SNI enabled" : "SNI disabled") << endl;
            }
            else
            {
                if (loc.mailbox.user != "")
                    cout << "CONFIG: Non-slot: " << "   user:        " << loc.mailbox.user << endl;
                cout << "CONFIG: Non-slot: " << "   destination: " << loc.mailbox.destination << endl;
            }
    }
}

