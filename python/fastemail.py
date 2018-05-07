#!/usr/bin/python3

import sys
import pwd
import grp
import os
import base64
import getopt
import json
import boto3
from email._header_value_parser import Domain
from setuptools.command.setopt import config_file
from cairo._cairo import Region

location_fmtstr = "/vmail/public/.%s"

#*********************************************************
class FastEmail:
    def __init__(self, email, config_file, mailrootdir, awsregion = "us-east-1", description = "", mail_uid = 5000, mail_gid = 5000):
        self._recipient = email
        email_parts = email.split('@')
        self._feed = email_parts[0]
        self._domain = email_parts[1]
        self._config = config_file
        self._region = awsregion
        self._uid = mail_uid
        self._gid = mail_gid
        if (mailrootdir.endswith("/")):
            self._mailroot = mailrootdir[:-1]
        else:
            self._mailroot = mailrootdir
            
        if (description == ""):
            self._description = feed.title() + " Mailbox"
        else:
            self._description = description
        
        self._SESclient = boto3.client('ses', self._region)
        self._SNSclient = boto3.client('sns', self._region)
        self._S3client = boto3.client('s3', self._region)

    def add_imap_mailbox(self, public = False, ruleset = "default-rule-set"):
        bucket_name = self._feed + "-" + self._domain.replace(".", "-")
        topic_name = bucket_name
        
        if (public):
            imap_dir = self._mailroot + "/public/." + feed
        else:
            imap_dir = self._mailroot + "/" + self._domain + "/" + self._recipient
            
        self.create_bucket(bucket_name)
        topic_arn = self.create_topic(topic_name)
        self.create_ses_rule(self._feed, self._recipient, topic_arn, bucket_name, ruleset)

    def create_bucket(self, bucket_name):
        # Create the bucket
        response = self._S3client.create_bucket(
            ACL='private',
            Bucket=bucket_name
            )

        if (response['ResponseMetadata']['HTTPStatusCode'] == 200):
            print("Successfully created S3 bucket " + bucket_name)
        else:
            print("Failed to create S3 bucket: " + response)

        response = self._S3client.put_bucket_policy( 
            Bucket=bucket_name,
            Policy="""{
                "Version": "2012-10-17",
                "Statement": 
                [
                    {
                        "Sid": "AllowSESPuts-1501418224586",
                        "Effect": "Allow",
                        "Principal": {
                            "Service": "ses.amazonaws.com"
                        },
                        "Action": "s3:PutObject",
                        "Resource": "arn:aws:s3:::""" + bucket_name + """/*",
                        "Condition": {
                            "StringEquals": {
                                "aws:Referer": "331557686640"
                            }
                        }
                    }
                ]
            }
            """
            )

        if (response['ResponseMetadata']['HTTPStatusCode'] == 204):
            print("Successfully set S3 policy for " + bucket_name)
        else:
            print("Failed to set S3 bucket policy: " + response)

        print(response)

    def create_topic(self, topic_name):
        # Create the topic
        response = self._SNSclient.create_topic(
            Name=topic_name
            )

        topic_arn = response['TopicArn'];
        if (response['ResponseMetadata']['HTTPStatusCode'] == 200):
            print("Successfully created SNS topic " + topic_name)
        else:
            print("Failed to create SNS topic: " + response)

        return topic_arn
    
    def create_ses_rule(self, feed, recipient, topic_arn, bucket_name, ruleset = "default-rule-set"):
        # get the name of the last rule in the rule set
        response = self._SESclient.describe_active_receipt_rule_set()
        last_rule_name = response['Rules'][len(response['Rules']) - 1]['Name']

        # Create the SES Rule in the default-rule-set
        response = self._SESclient.create_receipt_rule(
            RuleSetName=ruleset,
            After=last_rule_name,
            Rule={
                'Name': feed,
                'Enabled': True,
                'TlsPolicy': 'Optional',
                'Recipients': [
                    recipient 
                    ],
                'Actions': [
                    {
                        'S3Action': {
                            'TopicArn': topic_arn,
                            'BucketName': bucket_name
                            },
                    },
                ],
                'ScanEnabled': True
                }
            )

        if (response['ResponseMetadata']['HTTPStatusCode'] == 200):
            print("Successfully created SES rule \"" + feed + "\"")
        else:
            print("Failed to create SES rule: " + response)

    def read_local_config(self, config_file):
        os.setegid(self._gid)
        os.seteuid(self._uid)
        f = open(config_file, "r")
        cfg = f.read()
        f.close()
        config = json.loads( cfg )
        return (config)

    def write_local_config(self, config, config_file):
        os.setegid(self._gid)
        os.seteuid(self._uid)
        f = open(config_file, "w")
        jstr = json.dump(config, f, indent=4, separators=(',', ': '), ensure_ascii=False)
        f.close()
       
    def add_local_config(self, config, feed, recipient, topic_arn, bucket_name, enabled = True):
        new_mailbox = {"topic_arn"   : topic_arn,
                       "bucket"      : bucket_name,
                       "email"       : recipient,
                       "description" : self._description,
                       "enabled"     : enabled,
                       "locations"   : [] }
        
        pos = len(config["mailbox"])
        config["mailbox"].insert(pos, new_mailbox)

    def add_local_config_mailbox_location(self, config, email, public = False):
        location = { "type" : "mailbox", "public" : public }
        for i in range(0, len(config["mailbox"])):
            if (config["mailbox"][i]["email"] == email):
                config["mailbox"][i]["locations"].insert(0, location)


    def add_local_config_UCDP_location(self, email, ip):
        location = { "type" : "UCDP", "ip" : ip }
        for i in range(0, len(config["mailbox"])):
            if (config["mailbox"][i]["email"] == email):
                config["mailbox"][i]["locations"].insert(0, location)


    def add_local_config_forwarder_location(self, email, emaillist):
        location = { "type" : "forward", "email" : emaillist }
        for i in range(0, len(config["mailbox"])):
            if (config["mailbox"][i]["email"] == email):
                config["mailbox"][i]["locations"].insert(0, location)


    def restart_service(self):
        # Restart the emailfetch-from-s3 task
        ECSclient = boto3.client('ecs')
        response = ECSclient.describe_clusters( clusters=[ 'emailfetch-demo-cluster' ] )
        cluster_arn = response['clusters'][0]['clusterArn']
        response = ECSclient.list_tasks( cluster=cluster_arn, 
                                         serviceName='emailfetch-from-s3' )
        task_arn = response['taskArns'][0]
        ECSclient.stop_task( cluster=cluster_arn, task=task_arn )
        # the service will restart the task

#*********************************************************
def usage():
   print ("""
Usage: add_feed.py -f|--feed=<feed> -d|--domain=<domain> -c|--fetchcfg=<emailfetch config> [-h|--help]
   """)

#*********************************************************
def parse_argv():
   try:
       opts, args = getopt.getopt(sys.argv[1:], "hf:d:c:", ["help", "feed=", "domain=","fetchcfg="])
   except getopt.GetoptError as err:
       # print help information and exit:
       print(err) # will print something like "option -a not recognized"
       usage()
       sys.exit(2)
   feed = None
   domain = None
   for o, a in opts:
       if o in ("-h", "--help"):
           usage()
           sys.exit()
       elif o in ("-f", "--feed"):
           prefix = a
       elif o in ("-d", "--domain"):
           domain = a
       elif o in ("-c", "--fetchcfg"):
           fetchcfg = a
       else:
           assert False, "unhandled option"
   return { "feed":prefix, "domain":domain, "fetchcfg":fetchcfg }


#*********************************************************
if __name__ == "__main__":
   main()
