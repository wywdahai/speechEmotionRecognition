//
// Created by wangyanwei on 2021/6/3.
//

#ifdef __ANDROID__
#include <jni.h>
#include "speechEmotionRecognitionApi.h"

extern "C" {

JNIEXPORT jint JNICALL
Java_com_example_speechEmotionRecognition_SpeechEmotionRecognitionNative_speechEmotionRecognitionInit(
        JNIEnv *env, jobject thiz, jstring config_path, jstring model_path, jstring audio_dir,
        jstring feat_file_path) {
    const char *configPath = env->GetStringUTFChars(config_path, NULL);
    const char *modelPath = env->GetStringUTFChars(model_path, NULL);
    const char *audioDir = env->GetStringUTFChars(audio_dir, NULL);
    const char *featFilePath = env->GetStringUTFChars(feat_file_path, NULL);

    int ret = speechEmotionRecognitionInit(configPath, modelPath, audioDir, featFilePath);

    env->ReleaseStringUTFChars(config_path, configPath);
    env->ReleaseStringUTFChars(model_path, modelPath);
    env->ReleaseStringUTFChars(audio_dir, audioDir);
    env->ReleaseStringUTFChars(feat_file_path, featFilePath);

    return ret;
}

JNIEXPORT jstring JNICALL
Java_com_example_speechEmotionRecognition_SpeechEmotionRecognitionNative_speechEmotionPredict(
        JNIEnv *env, jobject thiz, jstring audio_path) {
    const char *audioPath = env->GetStringUTFChars(audio_path, NULL);

    char emotion_json[512] = {0};
    int ret = speechEmotionPredict(audioPath, emotion_json);

    env->ReleaseStringUTFChars(audio_path, audioPath);

    if (ret) return env->NewStringUTF("");

    return env->NewStringUTF(emotion_json);
}

}
#endif

