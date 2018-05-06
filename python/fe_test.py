#!/usr/bin/python

import requests
import pprint

url = 'http://localhost:10285/addmailbox'
data = """{"email":"feed4@testaws.pcifarelli.net",
           "ses_region":"us-east-1", 
           "enabled":"true", 
           "locations": [
               { "type" : "mailbox", "public":true },
               { "type" : "forward", "email" : [ "paul.cifarelli@thomsonreuters.com" ] }
	   ],
           "description":
             "Feed 4 Mailbox"}"""
headers = {"Content-Type": "application/json"}
resp = requests.post(url, headers=headers, data=data)
print("Status returned: ", resp.status_code);
if (resp.headers['content-type'] == 'application/json'):
   pp = pprint.PrettyPrinter(indent=4);
   pp.pprint(resp.json());
else:
   print(resp.text);

