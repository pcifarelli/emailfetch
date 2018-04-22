/*
 * Formatter.h
 *
 *  Created on: Aug 27, 2017
 *      Author: Paul Cifarelli
 *
 *      This simple formatter just saves each file using the object name as the file
 */

#ifndef SRC_FORMATTER_H_
#define SRC_FORMATTER_H_

#include <stdlib.h>
#include <aws/core/Aws.h>
#include <aws/s3/model/Object.h>
#include <fstream>
#include <ios>
#include <iterator>
#include <list>

typedef std::vector<std::string> mxservers_vector;
typedef std::map<int, mxservers_vector> mxbypref;   // we use a map (ordered) so that we iterate in order of the preference (which is the key)

namespace S3Downloader
{

class Formatter
{
public:
    Formatter(int verbose = 0);
    Formatter(mxbypref mxservers, int verbose = 0);
    Formatter(Aws::String dir, int verbose = 0);
    Formatter(Aws::String dir, mxbypref mxservers, int verbose = 0);
    virtual ~Formatter();

    virtual void open(const Aws::S3::Model::Object obj, const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    virtual std::string mkname(const Aws::S3::Model::Object obj) const;
    virtual void close();
    virtual Aws::OStream &getStream()
    {
        return m_local_file;
    }
    virtual Aws::OStream &getStream(void *buf, size_t sz);
    Aws::String &getName(void)
    {
        return m_name;
    }

    // override this member function to save in a different directory than you are tracking in the dirmap
    // for example, if you are saving (staging) in one dir and then moving to another
    virtual Aws::String &getSaveDir()
    {
        return m_dir;
    } // the directory we save into
    virtual Aws::String &getTrackDir()
    {
        return m_dir;
    } // the directory we are tracking

    // call after file is saved, in case some cleanup is needed
    virtual void clean_up()
    {
    }

    virtual void do_relay_forwarding(std::string fullpath);                  // guess at the envelope "to" based on the headers in the email
    virtual void do_relay_forwarding(std::string fullpath, std::string to);  // explicit envelope "to"

    // getKey is used to create a map of what we've already downloaded.  The key is the thing you want to use for comparisons.
    // you are passed the object and the filename to construct the key from
    // currently, it is not templated, because the only existing use cases are string keys
    virtual Aws::String getKey(Aws::String filename) const;

    void setDir(Aws::String dir)
    {
        m_dir = dir;
    }
    static int mkdirs(std::string dirname, mode_t mode);

private:
    std::stringstream m_ostream;
    int m_verbose;
    bool m_forward;
    mxbypref m_mxservers;

protected:
    double random_number();

    Aws::OFStream m_local_file;
    Aws::String m_name;		  // this is just the name of the file
    Aws::String m_fullpath;	  // this is the fullpath, including the filename
    Aws::String m_dir;          // directory to save to
    static mode_t m_newdirs_mode; // mode to use to create dirs
};

typedef std::list<S3Downloader::Formatter *> FormatterList;

}

#endif /* SRC_MAILFORMATTER_H_ */
