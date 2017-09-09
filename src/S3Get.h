/*
 * S3Get.h
 *
 *  Created on: Aug 26, 2017
 *      Author: paulc
 */

#ifndef SRC_S3GET_H_
#define SRC_S3GET_H_

#include <stdlib.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>
#include <ios>
#include "Formatter.h"

typedef Aws::Vector<Aws::S3::Model::Object> s3object_list;
typedef Aws::S3::Model::Object s3object;

class S3Get
{
public:
    S3Get() {};
    S3Get(Aws::String b_name);
    virtual ~S3Get();

    void listBuckets(void) const;
    void listObjects(void) const;
    void saveObjects(const Aws::String dir) const;
    void saveObjects(const Aws::String dir, Formatter &fmtr) const;
    void printObjects() const;
    s3object_list getObjectList(void) const;

    // save to a file using the object name as the filename
    bool objSaveAs(const Aws::S3::Model::Object obj, const Aws::String dir, Aws::String name) const;
    // save to a file using the Formatter to open the file
    bool objSaveAs(const Aws::S3::Model::Object obj, const Aws::String dir, const Aws::String name, Formatter &fmtr) const;
    // save it to a buffer
    bool objGet(const Aws::S3::Model::Object obj, char *buf, size_t sz) const;

private:
    Aws::S3::S3Client m_s3_client;
    Aws::String m_bucket_name;
};

#endif /* SRC_S3GET_H_ */
