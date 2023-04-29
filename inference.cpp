
#include <sys/uio.h>    // NOLINT(build/include_order)

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "inference.h"
#include "../include/tensorflow/lite/kernels/register.h"
#include "../include/tensorflow/lite/model.h"
#include "../include/tensorflow/lite/optional_debug_tools.h"
#include "../include/tensorflow/lite/string_util.h"
#include "speechEmotionRecognitionApi.h"
#include "featSMILExtract.h"

using namespace std;

#define LOG(x) std::cerr

static char emotion_list[][32] = {"Angry", "Fear", "Happiness", "Neutral", "Sadness", "Surprise", "Disgust"};

static bool is_speechEmotionRecognition_inited = false;

namespace stroke_model {

    class StrokeModel {
    public:
        StrokeModel(Settings *s);

        ~StrokeModel() {};

        std::vector<std::string> run_inference(std::vector<float> &inputs,
                                               std::vector<float> &class_probs);
        
        static void setModelPath(const char *path);
        static void setFeatFilePath(const char *path);
        
        static string getModelPath(void);
        static string getFeatFilePath(void);

    private:
        void read_word_to_index(const std::string &word_to_index_file);

        void read_index_to_label(const std::string &label_to_index_file);

        // model一定也要作为成员属性，model这个指针需要一些存在
        std::unique_ptr<tflite::FlatBufferModel> model;
        std::unique_ptr<tflite::Interpreter> cur_interpreter; // tflite 解释器

        // 模型推断时依赖的参数
        std::map<std::string, int> word_to_index;
        std::map<int, std::string> index_to_label;

        const int max_length = 70;
        const int num_classes = 7;
        
        static string modelPath;
        static string featFilePath;
    };

string StrokeModel::modelPath = "";
string StrokeModel::featFilePath = "";

void StrokeModel::setModelPath(const char *path)
{
    modelPath = path;
}

void StrokeModel::setFeatFilePath(const char *path)
{
    featFilePath = path;
}

string StrokeModel::getModelPath()
{
    return modelPath;
}

string StrokeModel::getFeatFilePath()
{
    return featFilePath;
}

/*
@param word_to_index_file：字母映射到索引的文件
@desc 读取字母到索引的映射表，写入到map中
*/
    void StrokeModel::read_word_to_index(const std::string &word_to_index_file) {
        std::ifstream infile;
        infile.open(word_to_index_file.data());
        assert(infile.is_open());

        std::string temp;
        while (getline(infile, temp)) {
            std::string::size_type pos = temp.find("\t");
            std::string word = temp.substr(0, pos);
            std::string index = temp.substr(pos + 1);
            int int_index = atoi(index.c_str());

            word_to_index.insert(std::pair<std::string, int>(word, int_index));
        }
        infile.close();
    }

/*
@param label_to_index_file：标签映射到索引的文件
@desc 读取字母到索引的映射表，写入到map中
*/
    void StrokeModel::read_index_to_label(const std::string &label_to_index_file) {
#ifdef READ_FROM_FILE
        std::ifstream infile;
        infile.open(label_to_index_file.data());
        assert(infile.is_open());

        std::string temp;
        while (getline(infile, temp)) {
            std::string::size_type pos = temp.find("\t");
            std::string word = temp.substr(0, pos);
            std::string index = temp.substr(pos + 1);
            int int_index = atoi(index.c_str());
            index_to_label.insert(std::pair<int, std::string>(int_index, word));
        }
        infile.close();
#endif
        index_to_label.insert(std::pair<int, std::string>(0, "Angry"));
        index_to_label.insert(std::pair<int, std::string>(1, "Fear"));
        index_to_label.insert(std::pair<int, std::string>(2, "Happiness"));
        index_to_label.insert(std::pair<int, std::string>(3, "Neutral"));
        index_to_label.insert(std::pair<int, std::string>(4, "Sadness"));
        index_to_label.insert(std::pair<int, std::string>(5, "Surprise"));
        index_to_label.insert(std::pair<int, std::string>(6, "Disgust"));
        
    }

/*
@param s: 模型推断依赖参数
@desc: 初始化时加载解释器和词索引表
*/
    StrokeModel::StrokeModel(Settings *s) {
        // 读取词索引表
//        read_word_to_index(s->word_to_index_file);
        read_index_to_label(s->label_to_index_file);

        if (!s->model_file.c_str()) {
            LOG(ERROR) << "no model file name\n";
            exit(-1);
        }

        // 1，创建模型和解释器对象，并加载模型

        model = tflite::FlatBufferModel::BuildFromFile(s->model_file.c_str());
        if (!model) {
            LOG(FATAL) << "\nFailed to mmap model " << s->model_file << "\n";
            exit(-1);
        }
        LOG(INFO) << "Loaded model " << s->model_file << "\n";
        model->error_reporter();
        LOG(INFO) << "resolved reporter\n";

        // 2，将模型中的tensor映射写入到解释器对象中
        tflite::ops::builtin::BuiltinOpResolver resolver;
        tflite::InterpreterBuilder(*model, resolver)(&cur_interpreter);

        if (!cur_interpreter) {
            LOG(FATAL) << "Failed to construct interpreter\n";
            exit(-1);
        }

        // interpreter->UseNNAPI(s->accel);
        // interpreter->SetAllowFp16PrecisionForFp32(s->allow_fp16);

        if (s->verbose) {
            LOG(INFO) << "tensors size: " << cur_interpreter->tensors_size() << "\n";
            LOG(INFO) << "nodes size: " << cur_interpreter->nodes_size() << "\n";
            LOG(INFO) << "inputs: " << cur_interpreter->inputs().size() << "\n";
            LOG(INFO) << "input(0) name: " << cur_interpreter->GetInputName(0) << "\n";
            LOG(INFO) << "input(0) dims: " << cur_interpreter->tensor(cur_interpreter->inputs()[0])->dims->size << "\n";
            LOG(INFO) << "input(0) data1: " << cur_interpreter->tensor(cur_interpreter->inputs()[0])->dims->data[1] << "\n";
            LOG(INFO) << "input(0) data2: " << cur_interpreter->tensor(cur_interpreter->inputs()[0])->dims->data[2] << "\n";
            LOG(INFO) << "input(0) data3: " << cur_interpreter->tensor(cur_interpreter->inputs()[0])->dims->data[3] << "\n";

            int t_size = cur_interpreter->tensors_size();
            for (int i = 0; i < t_size; i++) {
                if (cur_interpreter->tensor(i)->name)
                    LOG(INFO) << i << ": " << cur_interpreter->tensor(i)->name << ", "
                              << cur_interpreter->tensor(i)->bytes << ", "
                              << cur_interpreter->tensor(i)->type << ", "
                              << cur_interpreter->tensor(i)->params.scale << ", "
                              << cur_interpreter->tensor(i)->params.zero_point << "\n";
            }
        }

        if (s->number_of_threads != -1) {
            cur_interpreter->SetNumThreads(s->number_of_threads);
        }

        if (cur_interpreter->AllocateTensors() != kTfLiteOk) {
            LOG(FATAL) << "Failed to allocate tensors!";
        }

//    if (s->verbose) PrintInterpreterState(cur_interpreter.get());
    }

    std::vector<std::string> StrokeModel::run_inference(std::vector<float> &inputs,
                                                        std::vector<float> &class_probs) {

        std::vector<std::string> outputs;

        // 3，定义输入和输出tensor对象
        const std::vector<int> input_tensors = cur_interpreter->inputs();
        
//        std::vector<int> sizes = {1, 1, 1582};
        int input = input_tensors[0];
        
//        cur_interpreter->ResizeInputTensor(input, sizes);
//        cur_interpreter->AllocateTensors();
        
        float *input_data = cur_interpreter->typed_tensor<float>(input);
        
        // 1，将输入的单词转换成索引，并将数据写入到输入的tensor中
        for (int i=0;i<inputs.size();i++)
        {
            input_data[i] = inputs.at(i);
        }

        // 2，进行推断
        if (cur_interpreter->Invoke() != kTfLiteOk) {
            std::cout << "invoke failure" << std::endl;
        } else {
            std::cout << "invoke succesed" << std::endl;
        }

        // 3，获取结果
        TfLiteTensor *predict_tensor = cur_interpreter->tensor(cur_interpreter->outputs()[0]);
        float *predict_data = predict_tensor->data.f;
        
        //std::vector<float> class_probs;
        if (!predict_data)
            LOG(INFO) << "null pointer!\n";
        else{
            for (int j = 0; j < num_classes; j++){
                class_probs.push_back(predict_data[j]);
            }
            predict_data += num_classes;

            std::vector<float>::iterator max_val = std::max_element(std::begin(class_probs), std::end(class_probs));
            int max_index = std::distance(std::begin(class_probs), max_val);
            std::map<int, std::string>::iterator iter = index_to_label.find(max_index);
            std::string label = iter->second;
            outputs.push_back(label);
        }
        
        return outputs;
    }

}  // namespace stroke_model

int speechEmotionPredict(const char *audioPath, char *predictResult) {
    
    vector<float> featData;
    
    if (featExtract(audioPath, featData))
    {
        printf("featExtract excute failed..\n");
        return -1;
    }
    
    stroke_model::Settings s;
    s.model_file = stroke_model::StrokeModel::getModelPath();

    stroke_model::StrokeModel strokeModel(&s);
    
    std::vector<float> class_probs;
    std::vector<std::string> outputs = strokeModel.run_inference(featData, class_probs);
    
    std::cout << "outputs: ";
    for (int i = 0; i < outputs.size(); i++){
        std::cout << outputs[i] << " ";
    }
    std::cout << "\n";
    
    string predict_emotion;
    if (!class_probs.empty()){
        strcat(predictResult, "{");
        float maxWeight = 0.0;
        for (int i=0;i<7;i++)
        {
            char tmp[256] = {0};
            if (i == 6)
                sprintf(tmp, "\"%s\":%f", emotion_list[i], class_probs[i]);
            else
                sprintf(tmp, "\"%s\":%f,", emotion_list[i], class_probs[i]);
            strcat(predictResult, tmp);
            if (class_probs[i] > maxWeight)
            {
                maxWeight = class_probs[i];
                predict_emotion = emotion_list[i];
            }
        }
        strcat(predictResult, "}");
#if debug
        string strAudioPath = audioPath;
        int pos = strAudioPath.find_last_of("/");
        string audioDir = strAudioPath.substr(0, pos+1);
        string audioResultLogPath(audioDir);
        audioResultLogPath.append(predict_emotion).append(".log");
        //打开日志文件
        FILE *fd = fopen(audioResultLogPath.c_str(), "a+");
        if (NULL == fd)
        {
            printf("open %s failed\n", audioResultLogPath.c_str());
        }
        else{
            fwrite(predictResult, 1, strlen(predictResult), fd);
            fclose(fd);
        }
#endif
    }
    else
    {
        predictResult = NULL;
        return -1;
    }
    
    return 0;
}

int speechEmotionRecognitionInit(const char *configPath, const char *modelPath, const char *audioDir, const char *featFilePath)
{
    if (!is_speechEmotionRecognition_inited){
        is_speechEmotionRecognition_inited = true;
        stroke_model::StrokeModel::setModelPath(modelPath);
        stroke_model::StrokeModel::setFeatFilePath(featFilePath);

        //读取属性数据标准版数据
        string str(modelPath);
        int pos = str.find_last_of("/");
        string path = str.substr(0, pos+1);
        readMeanAndStdFromFile(path.append("mean_std_data.txt").c_str());
        
        return initOpenSMILE(configPath, audioDir, featFilePath);
    }
    
    return 0;
}

typedef struct {
    char *audioPath;
    speechEmotionPredictCallBack cb;
}ThreadFunParams;

void *thread_fun(void *param)
{
    char predictResult[512] = {0};
    ThreadFunParams *threadFunParams = (ThreadFunParams*)param;
    int ret = speechEmotionPredict(threadFunParams->audioPath, predictResult);
    
    threadFunParams->cb(predictResult);
    
    if (threadFunParams && threadFunParams->audioPath)
        delete []threadFunParams->audioPath;
    
    if (threadFunParams) delete threadFunParams;
    
    return 0;
}

int speechEmotionPredictWithCallBack(const char *audioPath, speechEmotionPredictCallBack cb)
{
    pthread_t pid;
    ThreadFunParams *threadFunParams = new ThreadFunParams;
    threadFunParams->audioPath = new char[strlen(audioPath)+1]();
    strcpy(threadFunParams->audioPath, audioPath);
    threadFunParams->cb = cb;
    
    int ret = pthread_create(&pid, NULL, thread_fun, (void*)threadFunParams);
    
    if (!ret)
    {
        printf("speechEmotionPredict thread create failed...\n");
    }
    
    return ret;
}
