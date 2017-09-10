/*
 * Downloader.cpp
 *
 *  Created on: Sep 9, 2017
 *      Author: paulc
 */

#include <dirent.h>
#include <sys/stat.h>

#include "Downloader.h"

Downloader::Downloader(const Aws::String dir, const int days, S3Get &s3, Formatter &fmt) :
    m_dir(dir), m_days(days), m_s3(s3), m_fmt(fmt)
{
    mkdirmap(dir.c_str(), days * SECONDS_PER_DAY);
}

Downloader::~Downloader()
{
}

void Downloader::saveNewObjects()
{
    s3object_list list = m_s3.getObjectList();
    std::time_t time_limit = m_days * SECONDS_PER_DAY;
    auto n = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(n);
    std::string dirname = m_dir.c_str();
    std::string c;

    std::size_t nn = m_dir.find_last_of('/');
    if (nn != std::string::npos && nn != m_dir.length() - 1)
        c = "/";

    dirname += c;

    if (!list.empty())
    {
        for (auto const &s3_object : list)
        {
            std::chrono::system_clock::time_point t = s3_object.GetLastModified().UnderlyingTimestamp();
            std::time_t ts = std::chrono::system_clock::to_time_t(t);

            if (now - ts < time_limit)
            {
                std::string fname = m_fmt.mkname(s3_object);
                Aws::String key = m_fmt.getKey(fname.c_str());
                std::string input = key.c_str();
                std::unordered_map<std::string,FileTracker>::const_iterator got = m_filemap.find (input);

                if ( got == m_filemap.end() ) // not found in map
                {
                    m_s3.objSaveAs(s3_object, m_dir, m_fmt);
                    std::string fullpath = dirname + fname;
                    FileTracker ft = { fullpath, ts };

                    std::pair<std::string,FileTracker> hashent (key.c_str(), ft);
                    m_filemap.insert(hashent);
                }
            }
        }
    }
}

int Downloader::mkdirmap(std::string dirname, time_t secs_back)
{
    DIR *dir;
    struct dirent *ent;
    struct dirent entry;
    struct stat fstats;
    std::string fname;
    std::string fullpath;
    std::string c;

    std::size_t n = dirname.find_last_of('/');
    if (n != std::string::npos && n != dirname.length() - 1)
        c = "/";

    if ((dir = opendir(dirname.c_str())) != NULL)
    {
        do
        {
            readdir_r(dir, &entry, &ent);
            if (ent)
            {
                fname = ent->d_name;
                fullpath = dirname + c + fname;
                if (!stat(fullpath.c_str(), &fstats))
                    if (S_ISREG(fstats.st_mode))
                    {
                        auto n = std::chrono::system_clock::now();
                        std::time_t now = std::chrono::system_clock::to_time_t(n);

                        if (now - fstats.st_mtim.tv_sec < secs_back)
                        {
                            Aws::String name = fname.c_str();
                            Aws::String key = m_fmt.getKey(name);
                            FileTracker ft = { fullpath, fstats.st_mtim.tv_sec };

                            std::pair<std::string, FileTracker> hashent (key.c_str(), ft);
                            m_filemap.insert(hashent);
                        }
                    }
            }
        } while (ent);
        closedir(dir);
    }
    else
    {
        /* could not open directory */
        return -1;
    }

    return 0;
}

void Downloader::purgeMap( std::time_t time_limit )
{
    auto n = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(n);
    std::vector<std::string> to_remove;

    for (auto it = m_filemap.begin(); it != m_filemap.end(); ++it)
    {
        if (now - it->second.ftime > time_limit)
            to_remove.push_back(it->first);
    }
    for (auto it = to_remove.begin(); it != to_remove.end(); ++it)
        m_filemap.erase(*it);
}

void Downloader::purgeMap()
{
    std::time_t time_limit = m_days * SECONDS_PER_DAY;
    purgeMap(time_limit);
}

void Downloader::printMap()
{
    std::time_t time_limit = m_days * SECONDS_PER_DAY;
    auto n = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(n);

    for ( auto it = m_filemap.begin(); it != m_filemap.end(); ++it )
    {
      std::cout << " " << it->first << ":" << it->second.ftime << std::endl;
    }
}
