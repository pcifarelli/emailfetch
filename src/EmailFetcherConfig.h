/*
 * EmailFetcherConfig.h
 *
 *  Created on: Nov 4, 2017
 *      Author: paulc
 */

#ifndef SRC_EMAILFETCHERCONFIG_H_
#define SRC_EMAILFETCHERCONFIG_H_

#include <aws/core/Aws.h>
#include <string>
#include <iterator>
#include "Downloader.h"

#include <aws/core/utils/json/JsonSerializer.h>

enum workflow_type
{
    NONSLOT,   // send to fast email
    SLOT,      // post to UCDP
    BOTH       // sending to fast email and posting to UCDP
};

struct location_type
{
    workflow_type type;
    std::string destination;
    struct
    {
        std::string user;
    } mailbox;
    struct
    {
        bool validate_json;
        std::string workdir;
        bool UCDP;
        unsigned short port;
        std::string snihostname;
        std::string certificate;
        std::string certificatepassword;
        std::string trclientid;
        std::string trfeedid;
        std::string trmessagetype;
        int         trmessageprio;
    } rest;
};
typedef std::list<location_type> location_list;
typedef std::vector<std::string> mxservers_vector;
typedef std::map<int, mxservers_vector> mxbyprio;
typedef std::map<std::string, mxbyprio> domain_forwarding_list;

struct config_item
{
    std::string name;                 // the name of this item (anything you want, for logging)
    std::string domainname;           // the domainname for this email - so that the email for this downloader is name@domainname
    std::string description;          // description, for logging/display purposes
    location_list locations;          // an array of locations to send to
    std::string topic_arn;            // the arn of the topic to wait on
    std::string bucket;               // the S3 bucket to download from
    S3Downloader::Downloader *pdownl; // the downloader object that does the downloading
    mxbyprio forward_servers;         // a vector of MX servers that also handle mail for this domain to forward all emails to
    bool enable_forwarding;           // enable forwarding to the forwarding_servers (one MX server chosen randomly for each entry in the list)
    bool has_nonslot_workflow;        // are any of the locations email workflow destinations?
    bool enabled;                     // is this mailbox enabled in the configuration
};

typedef std::list<config_item> config_list;

struct program_defaults
{
    uid_t euser;
    gid_t egroup;
    struct
    {
        std::string publicdir;
        std::string userdir;
    } mailbox_defaults;
    struct
    {
        bool        validate_json;
        std::string workdir;
        unsigned short port;
        std::string snihostname;
        std::string certificate;
        std::string certificatepassword;
        std::string trclientid;
        std::string trfeedid;
        std::string trmessagetype;
        int         trmessageprio;
    } UCDP_defaults;

    domain_forwarding_list forwarding_servers;
};

extern program_defaults defaults;

int get_config(const std::string location, program_defaults &defaults, config_list &config);
void print_config(config_list &mailboxconfig);

#endif /* SRC_EMAILFETCHERCONFIG_H_ */
