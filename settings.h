//
//  settings.h
//  TeacherClient
//
//  Created by 夏良芝 on 2021/5/13.
//  Copyright © 2021 stagekids. All rights reserved.
//

#ifndef settings_h
#define settings_h

#ifdef __ANDROID__

#include "tensorflow/lite/model.h"
#include "tensorflow/lite/string_type.h"
#else
#include "tensorflow/include/tensorflow/lite/model.h"
#include "tensorflow/include/tensorflow/lite/string_type.h"
#endif

namespace tflite {
namespace cpm {

struct Settings {
  bool verbose = false;
  bool accel = false;
  TfLiteType input_type = kTfLiteFloat32;
  bool profiling = false;
  bool allow_fp16 = false;
  bool gl_backend = false;
  bool hexagon_delegate = false;
  bool xnnpack_delegate = false;
  int loop_count = 1;
  float input_mean = 127.5f;
  float input_std = 127.5f;
  string model_name = "./mobilenet_quant_v1_224.tflite";
  tflite::FlatBufferModel* model;
  string input_bmp_name = "./grace_hopper.bmp";
  string labels_file_name = "./labels.txt";
  int number_of_threads = 4;
  int number_of_results = 5;
  int max_profiling_buffer_entries = 1024;
  int number_of_warmup_runs = 2;
};

}  // namespace label_image
}


#endif /* settings_h */
