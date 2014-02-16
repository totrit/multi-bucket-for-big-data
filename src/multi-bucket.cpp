/*
 * multi-bucket.cpp
 *
 *  Created on: Feb 16, 2014
 *      Author: totrit
 */
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int getTopN(const char* dataPath, int size, int N) {

}

int main() {
	string dataPath = "data/data.data";
	const uint64_t DATA_SIZE = 5 * 1024 * 1024 * 1024;
	const int BUF_LEN = 32 * 1024 * 1024;
	int *buf = new int[BUF_LEN];
	srand(time(NULL));
	ofstream out(dataPath);
	uint64_t randomed = 0;
	while(randomed < DATA_SIZE) {
		int toRandom = DATA_SIZE - randomed < BUF_LEN ? DATA_SIZE - randomed: BUF_LEN;
		for (int i = 0; i < toRandom; i ++) {
			buf[i] = rand();
		}
		out.write((const char*)buf, toRandom * (sizeof(int) / sizeof(char)));
		randomed += toRandom;
	}

}



