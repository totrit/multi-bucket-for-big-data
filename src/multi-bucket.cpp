/*
 * multi-bucket.cpp
 *
 *  Created on: Feb 16, 2014
 *      Author: totrit
 */
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;
typedef uint64_t large_num;
typedef uint value_type;
#define value_size sizeof(value_type)

large_num getFileSize(const char* filePath) {
  ifstream file(filePath, ios::binary | ios::ate);
  large_num ret = file.tellg();
  file.close();
  return ret;
}

static string getRelativePath(large_num index) {
  char numstr[21]; // enough to hold all numbers up to 64-bits
  sprintf(numstr, "%llu", index);
  return string("data/") + numstr + string(".data");
}

value_type getTopN(const char* dataPath, value_type rangeBegin, value_type rangeEnd, large_num topN) {
  //cout << "begin of getTopN: rangeBegin=" << rangeBegin << ", rangeEnd=" << rangeEnd << ", topN=" << topN << endl;
  //const large_num BUCKET_PROC_UNTIL_SIZE = 32 * 1024 * 1024;
  const large_num BUCKET_PROC_UNTIL_SIZE = 10 * 1024;
  large_num fileSize = getFileSize(dataPath);
  if (fileSize <= BUCKET_PROC_UNTIL_SIZE) {
    //cout << "doing the final sort." << endl;
    //If the processing file is small enough, we will turn to using quick sort.
    value_type *array = new value_type[fileSize / value_size];
    ifstream file(dataPath);
    file.read((char*)array, fileSize);
    vector<value_type> v(array, array + fileSize / value_size);
    sort(v.begin(), v.end());
    delete[] array;
    return v[topN - 1];
  } else {
    const char* tmpFileName = "data/tmp.data";
    large_num bucket_num = (large_num)sqrtl(fileSize / value_size);
    large_num sub_range_per_bucket = (rangeEnd - rangeBegin) / bucket_num; //TODO: sub_range_per_bucket may be larger than 'UINT_MAX'.
    //Rename the processing file to be the certain name 'tmp.data' to avoid file name conflicts during processing.
    rename(dataPath, tmpFileName);
    ifstream input;
    ofstream *outputs = new ofstream[bucket_num];
    input.open(tmpFileName);
    while(!input.eof()) {
      //1. Get an integer.
      value_type i;
      //damn silly 'eof' function, it will not report End Of File until i tried to read more than the file has.
      if (input.read((char*)&i, value_size).eof()) {
        break;
      }
      large_num bucket_index = (i - rangeBegin) / sub_range_per_bucket;
      //2. Conclude the bucket index that it belongs to.
      bucket_index = bucket_index >= bucket_num ? bucket_num - 1 : bucket_index;
      //3. Make sure the according file is open.
      if (!outputs[bucket_index].is_open()) {
        outputs[bucket_index].open(getRelativePath(bucket_index).c_str());
        if (!outputs[bucket_index].is_open()) {
          throw new exception();
        }
      }
      //4. Write the integer into the file accorded with the bucket index.
      outputs[bucket_index].write((char*)&i, value_size);
      //cout << "add " << i << " to bucket: " << bucket_index << endl;
    }
    large_num counter = 0;
    int64_t next_target = -1;
    for (large_num i = 0; i < bucket_num && counter < topN; i ++) {
      if (outputs[i].is_open()) {
        counter += outputs[i].tellp() / value_size;
        //cout << "searching next bucket: bucket_index=" << i << ", size=" << outputs[i].tellp() / value_size << ", counter=" << counter << endl;
      }
      next_target ++;
    }
    large_num testOriginSize = fileSize;
    //Close the processing file's handle and the handles of the outputed files.
    input.close();
    if (remove("data/tmp.data") != 0) {
      perror("error while deleting data/tmp.data.");
    }
    large_num next_target_file_size = 0;
    large_num testOutSize = 0;
    for(large_num i = 0; i < bucket_num; i ++) {
      if (outputs[i].is_open()) {
        if (i != next_target) {
          testOutSize += outputs[i].tellp();
          outputs[i].close();
          remove(getRelativePath(i).c_str());
        } else {
          next_target_file_size = outputs[i].tellp();
          testOutSize += next_target_file_size;
          outputs[i].close();
        }
      }
    }
    //cout << testOriginSize << "&" << testOutSize << endl;
    assert(testOriginSize == testOutSize);
    delete[] outputs;
    //Call recursivly for the 'next_target'.
    return getTopN(getRelativePath(next_target).c_str(), rangeBegin + next_target * sub_range_per_bucket,rangeBegin + next_target * sub_range_per_bucket + sub_range_per_bucket, topN - (counter - next_target_file_size / value_size));
  }
}

int main() {
  string dataPath = "data/data.data";
  //const large_num DATA_SIZE = 5 * 1024 * 1024 * 1024;
  const large_num DATA_SIZE = 512 * 1024;
  const large_num BUF_LEN = 32 * 1024 * 1024;
  value_type *buf = new value_type[BUF_LEN];
  srand(time(NULL));
  ofstream origDataFile;
  //TODO: should create data directory.
  origDataFile.open(dataPath.c_str());
  large_num randomed = 0;
  while(randomed < DATA_SIZE) {
    large_num toRandom = DATA_SIZE - randomed < BUF_LEN ? DATA_SIZE - randomed: BUF_LEN;
    for (large_num i = 0; i < toRandom; i ++) {
      buf[i] = rand() % INT32_MAX;
    }
    origDataFile.write((const char*)buf, toRandom * value_size);
    randomed += toRandom;
  }
  origDataFile.close();
  //Run our multi-bucket algo.
  value_type result_our = getTopN(dataPath.c_str(), 0, INT32_MAX, DATA_SIZE / 2);
  vector<value_type> v(buf, buf + DATA_SIZE);
  sort(v.begin(), v.end());
  value_type result_std = v[DATA_SIZE / 2 - 1];
  //Judge
  //cout << "our result: " << result_our << ", std result: " << result_std << endl;
  assert(result_std == result_our);
}
