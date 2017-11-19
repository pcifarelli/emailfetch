/*
 * UCDPDownloader.cpp
 *
 *  Created on: Oct 29, 2017
 *      Author: paulc
 */

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <stdio.h>
#include <cstddef>
#include <string>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <fstream>
#include <ios>
#include <sstream>
#include "UCDPFormatter.h"


UCDPFormatter::UCDPFormatter(Aws::String dir) : Formatter(dir)
{
    // TODO Auto-generated constructor stub

}

UCDPFormatter::~UCDPFormatter()
{
    // TODO Auto-generated destructor stub
}

void UCDPFormatter::open(const Aws::S3::Model::Object obj, const std::ios_base::openmode mode)
{
    m_objkey = obj.GetKey().c_str();
    m_message_drop_time = obj.GetLastModified();

    Formatter::open(obj, mode);
}

void UCDPFormatter::clean_up()
{
    std::string msgid;
    std::string to;
    std::string from;
    std::string subject;
    std::string date;
    Aws::String most_of_the_date;

    scan_headers(m_fullpath.c_str(), msgid, to, from, subject, date);
    // use rfc2822 msgid, if present.  It usually is, but it isn't strictly required
    if (!msgid.length())
        msgid = m_objkey;

    most_of_the_date = m_message_drop_time.ToGmtString("%Y-%m-%dT%H:%M:%S");
    int64_t secs_ms = m_message_drop_time.Millis(); // milliseconds since epoch
    std::chrono::system_clock::time_point time_se = m_message_drop_time.UnderlyingTimestamp();
    std::time_t t = std::chrono::system_clock::to_time_t(time_se);
    int64_t ms = secs_ms - ( t * 1000 );

    std::ostringstream o;
    o << most_of_the_date << '.' << std::setfill('0') << std::setw(7) << ms << "Z";
    date = o.str();

    std::cout << date << std::endl;
}

int UCDPFormatter::scan_headers(const std::string fname, std::string &msgid, std::string &to, std::string &from, std::string &subject, std::string &date)
{
    std::ifstream infile(fname, std::ifstream::in);

    std::string s;
    msgid = to = from = subject = "";
    std::regex e_msgid   ("^(Message-ID:).*<(.*)>(.*)");
    std::regex e_to      ("^(To:)(.*)");
    std::regex e_from    ("^(From:)(.*)");
    std::regex e_subject ("^(Subject:)(.*)");
    std::regex e_date    ("^(Date:)(.*)");
    std::smatch sm;
    while (infile.good() && !(msgid.length() && to.length() && from.length() && subject.length() && date.length()))
    {
       std::getline(infile, s);

       // chomp off any cr left over from windows text
       auto c = s.cend();
       c--;
       if (*c == '\r')
          s.erase(c);

       std::regex_match(s, sm, e_msgid);
       if (!msgid.length() && sm.size() > 0)
          msgid = sm[2];

       std::regex_match(s, sm, e_to);
       if (!to.length() && sm.size() > 0)
          to = sm[2];

       std::regex_match(s, sm, e_from);
       if (!from.length() && sm.size() > 0)
          from = sm[2];

       std::regex_match(s, sm, e_subject);
       if (!subject.length() && sm.size() > 0)
          subject = sm[2];

       std::regex_match(s, sm, e_date);
       if (!date.length() && sm.size() > 0)
          date = sm[2];
    }

    return 0;
}

std::string UCDPFormatter::escape_json(const std::string &s) 
{
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) 
    {
        switch (*c) 
        {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= *c && *c <= '\x1f') 
                {
                    o << "\\u"
                      << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
                } 
                else 
                {
                    o << *c;
                }
        }
    }
    return o.str();
}


std::string UCDPFormatter::unescape_json(const std::string &s) 
{
    std::ostringstream o;
    auto c = s.cbegin(); 
    while ( c != s.cend() )
    {
        if (*c == '\\')
        {
           auto p = c + 1;
           if (p != s.cend())
           {
              switch (*p)
              {
                  case '"' : o << '"';  break;
                  case '\\': o << '\\'; break;
                  case 'b' : o << '\b'; break;
                  case 'f' : o << '\f'; break;
                  case 'n' : o << '\n'; break;
                  case 'r' : o << '\r'; break;
                  case 't' : o << '\t'; break;
                  case 'u' :
                  {
                     if ( (p+1) != s.cend() &&
                          (p+2) != s.cend() &&
                          (p+3) != s.cend() &&
                          (p+4) != s.cend() )
                     {
                        char *stop;
                        std::string hex = "0x";
                        p++;
                        hex += *p;
                        p++;
                        hex += *p;
                        p++;
                        hex += *p;
                        p++;
                        hex += *p;
                        o << (char) strtol(hex.c_str(), &stop, 16);
                     }
                     else
                     {
                        o << '\\'; 
                        while ( p != s.cend() )
                        {
                           o << *p;
                           p++;
                        }
                     }
                  }
                  break;
              }
              c = p;
           }
           else
              o << '\\' << *c;
        }
        else
            o << *c;

        c++;
    }
    return o.str();
}
