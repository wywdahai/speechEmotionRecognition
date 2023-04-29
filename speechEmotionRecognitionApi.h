//
//  speechEmotionRecognitionApi.h
//  TeacherClient
//
//  Created by 夏良芝 on 2021/5/13.
//  Copyright © 2021 stagekids. All rights reserved.
//

#ifndef speechEmotionRecognitionApi_h
#define speechEmotionRecognitionApi_h

typedef void(*speechEmotionPredictCallBack)(void*);

#ifdef __cplusplus
extern "C"{
#endif

int speechEmotionPredict(const char *audioPath, char *predictResult);

int featExtractTest(const char *configPath, const char *audioPath, const char *featFilePath);

int speechEmotionRecognitionInit(const char *configPath, const char *modelPath, const char *audioDir, const char *featFilePath);

int speechEmotionPredictWithCallBack(const char *audioPath, speechEmotionPredictCallBack cb);

#ifdef __cplusplus
}
#endif

#endif /* speechEmotionRecognitionApi_h */
