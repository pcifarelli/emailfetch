#!/usr/bin/python

import sys
import pwd
import grp
import os
from os.path import *
import base64
import getopt
import json
import boto3

mail_uid = 5000
mail_gid = 5000
location_fmtstr = "/vmail/public/.%s"

#*********************************************************
def main():
   SESclient = boto3.client('ses')
   SNSclient = boto3.client('sns')
   S3client = boto3.client('s3')

   # parse arguments
   if len(sys.argv) > 1:
      opts = parse_argv()
      if opts["feed"] != None:
         feed = opts["feed"]
      if opts["domain"] != None:
         domain  = opts["domain"]
      if opts["fetchcfg"] != None:
         fetchcfg  = opts["fetchcfg"]

   bucket_name = feed + "-" + domain.replace(".", "-")
   topic_name = bucket_name
   recipient = feed + '@' + domain
   imap_dir = "/vmail/public/." + feed

   # Create the bucket
   response = S3client.create_bucket(
      ACL='private',
      Bucket=bucket_name
   )

   if (response['ResponseMetadata']['HTTPStatusCode'] == 200):
      print "Successfully created S3 bucket " + bucket_name
   else:
      print "Failed to create S3 bucket: " + response

   response = S3client.put_bucket_policy( 
      Bucket=bucket_name,
      Policy="""{
    "Version": "2012-10-17",
    "Statement": [
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
      print "Successfully set S3 policy for " + bucket_name
   else:
      print "Failed to set S3 bucket policy: " + response

   print response
   # Create the topic
   response = SNSclient.create_topic(
      Name=topic_name
   )

   topic_arn = response['TopicArn'];
   if (response['ResponseMetadata']['HTTPStatusCode'] == 200):
      print "Successfully created SNS topic " + topic_name
   else:
      print "Failed to create SNS topic: " + response

   # get the name of the last rule in the rule set
   response = SESclient.describe_active_receipt_rule_set()
   last_rule_name = response['Rules'][len(response['Rules']) - 1]['Name']


   # Create the SES Rule in the default-rule-set
   response = SESclient.create_receipt_rule(
      RuleSetName='default-rule-set',
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
#                      ,'ObjectKeyPrefix': 'string',
#                      'KmsKeyArn': 'string'
                  },
              },
          ],
          'ScanEnabled': True
      }
   )

   if (response['ResponseMetadata']['HTTPStatusCode'] == 200):
      print "Successfully created SES rule \"" + feed + "\""
   else:
      print "Failed to create SES rule: " + response

   os.setegid(mail_gid)
   os.seteuid(mail_uid)

   f = open(fetchcfg, "r")
   cfg = f.read()
   f.close()
   config = json.loads( cfg )

   new_mailbox = { "topic_arn" : topic_arn,
                   "bucket"    : bucket_name,
                   "name"      : feed.title() + " Mailbox",
                   "enabled"   : True,
                   "location"  : location_fmtstr % feed }
   pos = len(config["mailbox"])
   config["mailbox"].insert(pos, new_mailbox)
   f = open(fetchcfg, "w")
   jstr = json.dump(config, f, indent=4, separators=(',', ': '), ensure_ascii=False)
   f.close()

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
main()
