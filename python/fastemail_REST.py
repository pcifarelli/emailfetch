#!/usr/bin/python3
from setuptools.command.setopt import config_file
import nntplib

# REST Service for managing FastEmail
# creates:
# endpoint: /addmailbox 

application_name = "Thomson Reuters FastEmail"
default_port = 10285
default_mailbox_config = "/vmail/cfg/mailbox.json"

mailbox_config = default_mailbox_config
service_port = default_port
verbose = 0

import os
import glob
import time
import sys
import syslog
import getopt
import requests
import subprocess
from flask import Flask, url_for
from flask import Response
from flask import request
from flask import json

app = Flask(__name__)


@app.route('/')
def api_root():
    msg = 'Welcome to the FastEmail Configuration Service\n'
    msg += 'The config file is located at: ' + mailbox_config
    return msg

@app.route('/addmailbox', methods = ['POST'])
def api_addmailbox():
    if request.headers['Content-Type'] == 'application/json':
        msg = request.json
    else:
        return "415 Unsupported Media Type ;)"

    if (verbose):
        print (json.dumps(msg))
                
    data = { 'success' : '1' }
    js = json.dumps(data)
    resp = Response(js, status=200, mimetype='application/json')
    
    return resp

def parse_argv():
    try:
       opts, args = getopt.getopt(sys.argv[1:], "hm:p:v", ["help", "mailboxfile=", "port=", "verbose"])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
        
    mailboxfile = None
    port = None
    verbose = None
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-m", "--mailboxfile"):
            mailboxfile = a
        elif o in ("-p", "--port"):
            port = a
        elif o in ("-v", "--verbose"):
            verbose = a
        else:
            assert False, "unhandled option"
    return { "mailboxfile":mailboxfile, "verbose":verbose, "port":port }

def usage():
   print("""Usage: fastemail_REST.py [-m|--mailboxfile=<mailboxfile>] [-p|--port=<port>""")

# parse arguments
if len(sys.argv) > 1:
    opts = parse_argv()
    if opts["mailboxfile"] != None:
        hostsfile = opts["mailboxfile"]
    if opts["port"] != None:
        port = opt["port"]
    if opts["verbose"] != None:
        verbose=True

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=service_port)
	
