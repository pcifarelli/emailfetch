/*
 * Downloader.h
 *
 *  Created on: Sep 9, 2017
 *      Author: paulc
 */

#ifndef SRC_DOWNLOADER_H_
#define SRC_DOWNLOADER_H_

#include "S3Get.h"
#include <unordered_map>

#define SECONDS_PER_DAY 86400

class Downloader
{
public:
    // NOTE: s3 and fmt must stay in scope
    Downloader(const Aws::String dir, const int days, S3Get &s3, Formatter &fmt);
    virtual ~Downloader();

    // save only the objects that not already in the directory given by dir
    // NOTE: only the filename is of interest, the contents are not.  Therefore the content of the file can be empty
    // (this can be used to keep track of the objects that you have streamed elsewhere)
    // "days" - number of days back to check
    void saveNewObjects();
    void purgeMap();                     // purge m_days back
    void purgeMap( std::time_t secs_back );   // purge secs_back
    void printMap();

private:
    int mkdirmap(std::string dirname, time_t secs_back);

    Aws::String m_dir;
    Formatter &m_fmt;
    S3Get &m_s3;
    int m_days;

    struct FileTracker
    {
        std::string fname;
        std::time_t ftime;
    };
    std::unordered_map<std::string, FileTracker> m_filemap;
};
#endif /* SRC_DOWNLOADER_H_ */
