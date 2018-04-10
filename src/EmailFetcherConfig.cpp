/*
 * EmailFetcherConfig.cpp
 *
 *  Created on: Nov 4, 2017
 *      Author: paulc
 */
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <cstddef>
#include <string>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <fstream>
#include <ios>
#include <sstream>
#include <regex>

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
            false,
            "./mail/slot/%f",
            8301,
            "eapfastemail.ucdp.thomsonreuters.com",
            "./cfg/ucdp_eapfastemail.pem",
            "password",
            "eapfastemail",
            "news_eapfe",
            "EmailML",
            3
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
        if (ucdpdefaults.ValueExists("snihostname"))
            defaults.UCDP_defaults.snihostname = ucdpdefaults.GetString("snihostname").c_str();
        if (ucdpdefaults.ValueExists("certificate"))
            defaults.UCDP_defaults.certificate = ucdpdefaults.GetString("certificate").c_str();
        if (ucdpdefaults.ValueExists("certificatepassword"))
            defaults.UCDP_defaults.certificatepassword = ucdpdefaults.GetString("certificatepassword").c_str();
        if (ucdpdefaults.ValueExists("trclientid"))
            defaults.UCDP_defaults.trclientid = ucdpdefaults.GetString("trclientid").c_str();
        if (ucdpdefaults.ValueExists("trfeedid"))
            defaults.UCDP_defaults.trfeedid = ucdpdefaults.GetString("trfeedid").c_str();
        if (ucdpdefaults.ValueExists("trmessagetype"))
            defaults.UCDP_defaults.trmessagetype = ucdpdefaults.GetString("trmessagetype").c_str();
        if (ucdpdefaults.ValueExists("workdir"))
            defaults.UCDP_defaults.workdir = ucdpdefaults.GetString("workdir").c_str();
        if (ucdpdefaults.ValueExists("trmessagepriority"))
            defaults.UCDP_defaults.trmessageprio = ucdpdefaults.GetInteger("trmessagepriority");
        if (ucdpdefaults.ValueExists("validate_json"))
            defaults.UCDP_defaults.validate_json = ucdpdefaults.GetString("validate_json").c_str();
    }
    if (jdefaults.ValueExists("MX_forwarding_servers"))
    {
        Utils::Array<Utils::Json::JsonValue> domain_arr = jdefaults.GetArray("MX_forwarding_servers");
        for (int i = 0; i < domain_arr.GetLength(); i++)
        {
            string domain = domain_arr[i].GetString("domain").c_str();
            Utils::Array<Utils::Json::JsonValue> mx_arr = domain_arr[i].GetArray("mx");

            if (defaults.forwarding_servers.find(domain) == defaults.forwarding_servers.end())
            {
                mxbypref m;
                std::pair<std::string, mxbypref> ent(domain, m);
                defaults.forwarding_servers.insert(ent);
            }

            for (int j = 0; j < mx_arr.GetLength(); j++)
            {
                if (mx_arr[j].ValueExists("pref") && mx_arr[j].ValueExists("server"))
                {
                    int pref = mx_arr[j].GetInteger("pref");
                    string server = mx_arr[j].GetString("server").c_str();
                    auto mxent = defaults.forwarding_servers[domain].find(pref);
                    if (mxent != defaults.forwarding_servers[domain].end())
                        mxent->second.push_back(server);
                    else
                    {
                        mxservers_vector mxservers;
                        mxservers.push_back(server);

                        std::pair<int, mxservers_vector> ent(pref, mxservers);
                        defaults.forwarding_servers[domain].insert(ent);
                    }
                }
            }
        }
    }
    if (jdefaults.ValueExists("Forwarder"))
    {
        Utils::Json::JsonValue fwddefaults = jdefaults.GetObject("Forwarder");
        if (fwddefaults.ValueExists("workdir"))
            defaults.Forwarder_defaults.workdir = fwddefaults.GetString("workdir").c_str();
        if (fwddefaults.ValueExists("from"))
            defaults.Forwarder_defaults.from = fwddefaults.GetString("from").c_str();
    }
    if (jdefaults.ValueExists("mailout_servers"))
    {
        Utils::Array<Utils::Json::JsonValue> mailout_arr = jdefaults.GetArray("mailout_servers");
        for (int i = 0; i < mailout_arr.GetLength(); i++)
        {
            outgoing_mail_server oms;

            string domain = "";
            if (mailout_arr[i].ValueExists("domain"))
                domain = mailout_arr[i].GetString("domain").c_str();

            oms.server = "";
            if (mailout_arr[i].ValueExists("server"))
                oms.server = mailout_arr[i].GetString("server").c_str();

            oms.username = "";
            if (mailout_arr[i].ValueExists("username"))
                oms.username = mailout_arr[i].GetString("username").c_str();

            oms.password = "";
            if (mailout_arr[i].ValueExists("password"))
                oms.password = mailout_arr[i].GetString("password").c_str();

            oms.port = 587;
            if (mailout_arr[i].ValueExists("port"))
                oms.port = mailout_arr[i].GetInteger("port");

            oms.tls = true;
            if (mailout_arr[i].ValueExists("tls"))
                oms.tls = mailout_arr[i].GetBool("tls");

            std::pair<std::string, outgoing_mail_server> ent(domain, oms);
            defaults.mailout_servers.insert(ent);
        }
    }
}

string replace_all(string str, const string &from, const string &to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

string create_location(string fmt, string &user, string &name, string &domainname, string destination)
{
    string result = fmt;

    result = replace_all(result, "%u", user);        // replace all "%u with the user's name
    result = replace_all(result, "%f", name);        // replace all "%f with the feed name
    result = replace_all(result, "%d", domainname);  // replace all "%d with the domain name
    result = replace_all(result, "%a", destination); // replace all "%a with the destination ip or host name

    std::size_t n = result.find_last_of('/');
    if (n != std::string::npos && n != result.length() - 1)
        result += "/";

    return result;
}

string create_public_mailbox_location(program_defaults &defaults, string &name, string &domainname)
{
    return create_location(defaults.mailbox_defaults.publicdir, name, name, domainname, "");
}

string create_user_mailbox_location(program_defaults &defaults, string &user, string &name, string &domainname)
{
    return create_location(defaults.mailbox_defaults.userdir, user, name, domainname, "");
}

void get_mailbox_config(Utils::Json::JsonValue &jv, config_list &config)
{
    Utils::Array<Utils::Json::JsonValue> arr = jv.GetArray("mailbox");

    for (int i = 0; i < arr.GetLength(); i++)
    {
        config_item item;

        if (arr[i].ValueExists("bucket"))
            item.bucket = arr[i].GetString("bucket").c_str();
        else
        {
            cout << "CONFIG: Error: no AWS S3 bucket specified for this downloader -  ignoring" << endl;
            continue;
        }

        if (arr[i].ValueExists("email"))
        {
            string email = arr[i].GetString("email").c_str();
            istringstream e(email);
            getline(e, item.name, '@');
            getline(e, item.domainname, '@');
        }
        else
        {
            if (arr[i].ValueExists("name"))
                item.name = arr[i].GetString("name").c_str();
            else
            {
                cout << "CONFIG: Error: Cannot determine the email address for this downloader (name missing)" << endl;
                continue;
            }
            if (arr[i].ValueExists("domainname"))
                item.domainname = arr[i].GetString("domainname").c_str();
            else
            {
                cout << "CONFIG: Error: Cannot determine the email address for this downloader (domainname missing)" << endl;
                continue;
            }
        }

        if (arr[i].ValueExists("description"))
            item.description = arr[i].GetString("description").c_str();
        else
            item.description = item.name + "@" + item.domainname;

        if (arr[i].ValueExists("topic_arn"))
            item.topic_arn = arr[i].GetString("topic_arn").c_str();
        else
        {
            cout << "CONFIG: Error: no topic arn specified for downloader " << item.name << endl;
            continue;
        }

        Utils::Array<Utils::Json::JsonValue> locations = arr[i].GetArray("locations");
        int nlocs = locations.GetLength();
        for (int j = 0; j < nlocs; j++)
        {
            if (locations[j].ValueExists("type"))
            {
                location_type loc;
                string type = "";

                type = locations[j].GetString("type").c_str();
                if (EmailExtractor::str_tolower(type) == "mailbox")
                {
                    loc.type = MAILBOX;
                    item.has_nonslot_workflow = true;
                    bool is_public = false;
                    string user = "";
                    string workdir = "";

                    if (locations[j].ValueExists("public"))
                        is_public = locations[j].GetBool("public");
                    if (locations[j].ValueExists("user"))
                        user = locations[j].GetString("user").c_str();
                    if (locations[j].ValueExists("workdir"))
                        workdir = locations[j].GetString("workdir").c_str();

                    if (is_public)
                        loc.mailbox.destination = create_public_mailbox_location(defaults, item.name, item.domainname);
                    else
                    {
                        if (user != "")
                        {
                            loc.mailbox.user = user;
                            loc.mailbox.destination = create_user_mailbox_location(defaults, loc.mailbox.user, item.name, item.domainname);
                        }
                        else
                        {
                            if (workdir != "")
                            {
                                string fmt = workdir;
                                loc.mailbox.user = "";
                                loc.mailbox.destination = create_location(fmt, user, item.name, item.domainname, "");
                            }
                            else
                            {
                                cout
                                    << "CONFIG: Error: Unable to find user or workdir element for mailbox location - ignoring this location"
                                    << endl;
                                continue;
                            }
                        }
                    }
                }
                else if (EmailExtractor::str_tolower(type) == "ucdp")// it goes to a UCDP REST post
                {
                    string addr = "";

                    loc.type = UCDP;
                    // SNI is the default for UCDP, and UCDP is the default system to post to
                    if (locations[j].ValueExists("UCDP"))
                        loc.rest.UCDP = locations[j].GetBool("UCDP");
                    else
                        loc.rest.UCDP = true;

                    if (!loc.rest.UCDP && locations[j].ValueExists("url"))
                    {
                        string fmt = locations[j].GetString("url").c_str();
                        string user = "";
                        loc.rest.destination = create_location(fmt, user, item.name, item.domainname, "");

                        // extract the ip or host from the url, for substitutions
                        std::smatch sm;
                        std::regex e_addr("^(http[s]?:)//([^/]+)/(.*)");
                        std::regex_match(loc.rest.destination, sm, e_addr);
                        if (sm.size() > 0)
                            addr = sm[2];

                    }
                    else if (loc.rest.UCDP && locations[j].ValueExists("ip"))  // these are the minimum required params for SNI
                    {
                        loc.rest.destination = locations[j].GetString("ip").c_str();
                        addr = loc.rest.destination;

                        if (locations[j].ValueExists("certificate"))
                            loc.rest.certificate = locations[j].GetString("certificate").c_str();
                        else
                            loc.rest.certificate = defaults.UCDP_defaults.certificate;

                        if (locations[j].ValueExists("certificatepassword"))
                            loc.rest.certificatepassword = locations[j].GetString("certificatepassword").c_str();
                        else
                            loc.rest.certificatepassword = defaults.UCDP_defaults.certificatepassword;

                        if (locations[j].ValueExists("port"))
                            loc.rest.port = locations[j].GetInteger("port");
                        else
                            loc.rest.port = defaults.UCDP_defaults.port;

                        if (locations[j].ValueExists("snihostname"))
                            loc.rest.snihostname = locations[j].GetString("snihostname").c_str();
                        else
                            loc.rest.snihostname = defaults.UCDP_defaults.snihostname;

                        if (locations[j].ValueExists("trclientid"))
                            loc.rest.trclientid = locations[j].GetString("trclientid").c_str();
                        else
                            loc.rest.trclientid = defaults.UCDP_defaults.trclientid;

                        if (locations[j].ValueExists("trfeedid"))
                            loc.rest.trfeedid = locations[j].GetString("trfeedid").c_str();
                        else
                            loc.rest.trfeedid = defaults.UCDP_defaults.trfeedid;

                        if (locations[j].ValueExists("trmessagetype"))
                            loc.rest.trmessagetype = locations[j].GetString("trmessagetype").c_str();
                        else
                            loc.rest.trmessagetype = defaults.UCDP_defaults.trmessagetype;

                        if (locations[j].ValueExists("trmessagepriority"))
                            loc.rest.trmessageprio = locations[j].GetInteger("trmessagepriority");
                        else
                            loc.rest.trmessageprio = defaults.UCDP_defaults.trmessageprio;

                        if (locations[j].ValueExists("validate_json"))
                            loc.rest.validate_json = locations[j].GetBool("validate_json");
                        else
                            loc.rest.validate_json = defaults.UCDP_defaults.validate_json;
                    }
                    else
                    {
                        cout << "CONFIG: Error: no URL or IP/HOST/PORT specified for REST destination - ignoring location in "
                            << item.name << endl;
                        continue;
                    }

                    if (locations[j].ValueExists("workdir"))
                    {
                        string fmt = locations[j].GetString("workdir").c_str();
                        loc.rest.workdir = create_location(fmt, loc.mailbox.user, item.name, item.domainname, addr);
                    }
                    else
                        loc.rest.workdir = create_location(defaults.UCDP_defaults.workdir, loc.mailbox.user, item.name,
                            item.domainname, addr);
                }
                else if (EmailExtractor::str_tolower(type) == "forward")// it goes to a UCDP REST post
                {
                    loc.type = FORWARD;
                    map<string,outgoing_mail_server>::iterator it = defaults.mailout_servers.find(item.domainname);
                    outgoing_mail_server oms = {"", "", "", 587, true};

                    if (it != defaults.mailout_servers.end())
                        oms = it->second;

                    loc.forwarder.server_info = oms;
                    if (locations[j].ValueExists("server"))
                        loc.forwarder.server_info.server = locations[j].GetString("server").c_str();

                    if (locations[j].ValueExists("username"))
                        loc.forwarder.server_info.username = locations[j].GetString("username").c_str();

                    if (locations[j].ValueExists("password"))
                        loc.forwarder.server_info.password = locations[j].GetString("password").c_str();

                    if (locations[j].ValueExists("port"))
                        loc.forwarder.server_info.port = locations[j].GetInteger("port");

                    if (locations[j].ValueExists("tls"))
                        loc.forwarder.server_info.tls = locations[j].GetBool("tls");

                    string dummy = "";
                    loc.forwarder.workdir = create_location(defaults.Forwarder_defaults.workdir, dummy, item.name, item.domainname, "");
                    loc.forwarder.from = create_location(defaults.Forwarder_defaults.from, dummy, item.name, item.domainname, "");

                    if (locations[j].ValueExists("workdir"))
                    {
                        string fmt = locations[j].GetString("workdir").c_str();
                        loc.forwarder.workdir = create_location(fmt, dummy, item.name, item.domainname, "");
                    }
                    if (locations[j].ValueExists("from"))
                    {
                        string fmt = locations[j].GetString("from").c_str();
                        loc.forwarder.from = create_location(fmt, dummy, item.name, item.domainname, "");
                    }
                    if (locations[j].ValueExists("email")) // otherwise we are wasting our time
                    {
                        Utils::Array<Utils::Json::JsonValue> emails = locations[j].GetArray("email");

                        for (int k=0; k < emails.GetLength(); k++)
                        {
                            string fmt = emails[k].AsString().c_str();
                            string destination = create_location(fmt, loc.mailbox.user, item.name, item.domainname, "");

                            loc.forwarder.email_destinations.push_back(destination);
                        }
                    }
                    else
                        cout << "CONFIG: Warning: \"forwarder\" location type specified without any forwarding email addresses" << std::endl;
                }
                else if (EmailExtractor::str_tolower(type) == "url")// it goes to a UCDP REST post
                {
                    loc.type = URL;
                    cout << "CONFIG: Warning: \"url\" location type not yet implemented" << std::endl;
                }
                else
                {
                    loc.type = UNKNOWN;
                    cout << "CONFIG: Error: unknown location type" << std::endl;
                }

                item.locations.push_back(loc);
            }
            else
                cout << "CONFIG: Error: required element \"mailbox\" boolean element not specified - ignoring location in "
                    << item.name << endl;
        }

        item.enabled = arr[i].GetBool("enabled");

        item.enable_mxforwarding = false;
        if (arr[i].ValueExists("mxforward"))
        {
            item.enable_mxforwarding = arr[i].GetBool("mxforward");
            if (item.enable_mxforwarding && (defaults.forwarding_servers.find(item.domainname) != defaults.forwarding_servers.end()))
                item.forward_servers = defaults.forwarding_servers.find(item.domainname)->second;
            else
            {
                // no mx forwarding servers defined for this domain; disable server forwarding
                item.enable_mxforwarding = false;
                cout << "CONFIG: Error: MX forwarding enabled for " << item.name << '@' << item.domainname << " without any MX servers defined for the domain" << std::endl;
            }
        }

        config.push_back(item);
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
    //cout << "MX FORWARDING LIST:" << endl;
    // print the forwarding list
    //for (auto &mxservers : defaults.forwarding_servers)
    //{
    //    cout << "   DOMAIN: " << mxservers.first << endl;
    //    for (auto &mxent : mxservers.second)
    //    {
    //        cout << "      PREF: " << mxent.first << endl;
    //        for (auto &server : mxent.second)
    //            cout << "         MXSERVER: " << server << endl;
    //    }
    //}

    //cout << "DEFAULT OUTGOING MAILSERVERS" << endl;
    //for (auto &serverdetails : defaults.mailout_servers)
    //{
    //    cout << "   DOMAIN: " << serverdetails.first << endl;
    //    cout << "   SERVER: " << serverdetails.second.server << endl;
    //    cout << "   USER:   " << serverdetails.second.username << endl;
    //    cout << "   PASSWD: " << serverdetails.second.password << endl;
    //    cout << "   PORT:   " << serverdetails.second.port << endl;
    //    cout << "   TLS?:   " << (serverdetails.second.tls? "YES" : "NO") << endl;
    //}

    // print the mailboxes
    for (auto &item : mailboxconfig)
    {
        cout << "CONFIG: " << item.description << endl;
        cout << "CONFIG:    " << item.name << '@' << item.domainname;
        cout << (item.enabled ? " is enabled" : " is disabled");
        cout << (item.has_nonslot_workflow ? " and has non-slot workflow\n" : "\n");
        cout << "CONFIG:    " << "S3 Bucket:     " << item.bucket << endl;
        cout << "CONFIG:    " << "SNS Topic ARN: " << item.topic_arn << endl;
        if (item.enable_mxforwarding)
        {
            cout << "CONFIG:    Forwarding enabled" << endl;
            cout << "CONFIG:       MX Servers:" << endl;
            for (auto &mxent : item.forward_servers)
            {
                cout << "CONFIG:       PREF: " << mxent.first << endl;
                for (auto &server : mxent.second)
                    cout << "CONFIG:          MXSERVER: " << server << endl;
            }
        }
        else
            cout << "CONFIG:    Forwarding disabled" << endl;
        cout << "CONFIG:    " << "Locations:" << endl;
        int i = 0;
        for (auto &loc : item.locations)
        {
            ++i;
            if (loc.type == UCDP)
            {
                if (loc.rest.UCDP)
                {
                    cout << "CONFIG:       " << i << ". Type:  " << "SLOT: UCDP destination" << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   IP:            " << loc.rest.destination << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   Port:          " << loc.rest.port << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   ClientId:      " << loc.rest.trclientid << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   SNIHostName:   " << loc.rest.snihostname << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   FeedId:        " << loc.rest.trfeedid << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   MessageType:   " << loc.rest.trmessagetype << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   MessagePrio:   " << loc.rest.trmessageprio << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   ValidateJson?: " << (loc.rest.validate_json?"true":"false") << endl;
                }
                else
                {
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "SLOT: REST URL destination" << endl;
                    cout << "CONFIG:       " << i << ".    UCDP:  " << "   URL:         " << loc.rest.destination << endl;
                }
                cout << "CONFIG:       " << i << ".    UCDP:  " << "   workdir:       " << loc.rest.workdir << endl;
            }
            else if (loc.type == MAILBOX)
            {
                cout << "CONFIG:       " << i << ". Type:  " << "NON-SLOT Mailbox Destination" << endl;
                if (loc.mailbox.user != "")
                    cout << "CONFIG:       " << i << ".    Mailbox:" << "  user:          " << loc.mailbox.user << endl;
                cout << "CONFIG:       " << i << ".    Mailbox:" << "  destination:   " << loc.mailbox.destination << endl;
            }
            else if (loc.type == URL)
            {
                cout << "CONFIG:       " << i << ". Type:  " << "URL Destination" << endl;
                cout << "CONFIG:       " << i << ".    URL:   " << loc.mailbox.destination << endl;
            }
            else if (loc.type == FORWARD)
            {
                cout << "CONFIG:       " << i << ". Type:  " << "Forward Email Destination" << endl;
                cout << "CONFIG:       " << i << ".    Workdir:  " << loc.forwarder.workdir << endl;
                cout << "CONFIG:       " << i << ".    Username: " << loc.forwarder.server_info.username << endl;
                //cout << "CONFIG:       " << i << ".    Password: " << loc.forwarder.password << endl;
                cout << "CONFIG:       " << i << ".    Server:   " << loc.forwarder.server_info.server << endl;
                cout << "CONFIG:       " << i << ".    Port:     " << loc.forwarder.server_info.port << endl;
                cout << "CONFIG:       " << i << ".    TLS?:     " << (loc.forwarder.server_info.tls ? "Yes" : "No") << endl;
                for (auto &email : loc.forwarder.email_destinations)
                {
                    cout << "CONFIG:       " << i << ".    Email:    " << email << endl;
                }
            }
            else
                cout << "CONFIG:       " << i << ". Type:  " << "UNKNOWN" << endl;
        }
    }
}

