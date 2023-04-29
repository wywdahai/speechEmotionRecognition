//
// Created by wangyanwei on 2021/5/12.
//

#ifndef VIDEO_RECORD_FEATSMILEXTRACT_H
#define VIDEO_RECORD_FEATSMILEXTRACT_H

#include <vector>

using namespace std;

int featExtractTest(const char *configPath, const char *audioPath, const char *featFilePath, vector<float> &featData);

int initOpenSMILE(const char *configPath, const char *audioDir, const char *featFilePath);

int featExtract(const char *audioPath, vector<float> &ret);

bool readMeanAndStdFromFile(const char *filePath);

#endif //VIDEO_RECORD_FEATSMILEXTRACT_H
