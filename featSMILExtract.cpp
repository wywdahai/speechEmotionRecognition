/*F***************************************************************************
 * 
 * openSMILE - the Munich open source Multimedia Interpretation by 
 * Large-scale Extraction toolkit
 * 
 * This file is part of openSMILE.
 * 
 * openSMILE is copyright (c) by audEERING GmbH. All rights reserved.
 * 
 * See file "COPYING" for details on usage rights and licensing terms.
 * By using, copying, editing, compiling, modifying, reading, etc. this
 * file, you agree to the licensing terms in the file COPYING.
 * If you do not agree to the licensing terms,
 * you must immediately destroy all copies of this file.
 * 
 * THIS SOFTWARE COMES "AS IS", WITH NO WARRANTIES. THIS MEANS NO EXPRESS,
 * IMPLIED OR STATUTORY WARRANTY, INCLUDING WITHOUT LIMITATION, WARRANTIES OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ANY WARRANTY AGAINST
 * INTERFERENCE WITH YOUR ENJOYMENT OF THE SOFTWARE OR ANY WARRANTY OF TITLE
 * OR NON-INFRINGEMENT. THERE IS NO WARRANTY THAT THIS SOFTWARE WILL FULFILL
 * ANY OF YOUR PARTICULAR PURPOSES OR NEEDS. ALSO, YOU MUST PASS THIS
 * DISCLAIMER ON WHENEVER YOU DISTRIBUTE THE SOFTWARE OR DERIVATIVE WORKS.
 * NEITHER TUM NOR ANY CONTRIBUTOR TO THE SOFTWARE WILL BE LIABLE FOR ANY
 * DAMAGES RELATED TO THE SOFTWARE OR THIS LICENSE AGREEMENT, INCLUDING
 * DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL OR INCIDENTAL DAMAGES, TO THE
 * MAXIMUM EXTENT THE LAW PERMITS, NO MATTER WHAT LEGAL THEORY IT IS BASED ON.
 * ALSO, YOU MUST PASS THIS LIMITATION OF LIABILITY ON WHENEVER YOU DISTRIBUTE
 * THE SOFTWARE OR DERIVATIVE WORKS.
 * 
 * Main authors: Florian Eyben, Felix Weninger, 
 * 	      Martin Woellmer, Bjoern Schuller
 * 
 * Copyright (c) 2008-2013, 
 *   Institute for Human-Machine Communication,
 *   Technische Universitaet Muenchen, Germany
 * 
 * Copyright (c) 2013-2015, 
 *   audEERING UG (haftungsbeschraenkt),
 *   Gilching, Germany
 * 
 * Copyright (c) 2016,	 
 *   audEERING GmbH,
 *   Gilching Germany
 ***************************************************************************E*/

/*

This is the main commandline application

*/

//#include <core/smileCommon.hpp>

#include <core/configManager.hpp>
#include <core/commandlineParser.hpp>
#include <core/componentManager.hpp>
#include <iocore/waveSource.hpp>

#include "featSMILExtract.h"
#include "meanAndStdData.h"
#include "speechEmotionRecognitionApi.h"
#include <fstream>

#define MODULE "SMILExtract"

#define LINE_SIZE  30000
#define READ_BYTE_FROM_FILE_OF_EACH 1024

/************** Ctrl+C signal handler **/
#include  <signal.h>
#include <string>

using namespace std;

cComponentManager *cmanGlob = NULL;
cConfigManager *configManager = NULL;
cComponentManager *cMan = NULL;
bool is_inited_openSMILE = false;
string g_audioSavePath  = "sample.wav";
string g_featFilePath = "";

void INThandler(int);
int ctrlc = 0;

void INThandler(int sig)
{
  signal(sig, SIG_IGN);
  if (cmanGlob != NULL) cmanGlob->requestAbort();
  signal(SIGINT, INThandler);
  ctrlc = 1;
}
/*******************************************/

int initOpenSMILEConfig(int argc, const char *argv[])
{
    try {

      //smileCommon_fixLocaleEnUs();

      // set up the smile logger
      LOGGER.setLogLevel(1);
      LOGGER.enableConsoleOutput();


      // commandline parser:
      cCommandlineParser cmdline(argc, (char**)argv);
      cmdline.addStr( "configfile", 'C', "Path to openSMILE config file", "smile.conf" );
      cmdline.addInt( "loglevel", 'l', "Verbosity level (0-9)", 2 );
  #ifdef DEBUG
      cmdline.addBoolean( "debug", 'd', "Show debug messages (on/off)", 0 );
  #endif
      cmdline.addInt( "nticks", 't', "Number of ticks to process (-1 = infinite) (only works for single thread processing, i.e. nThreads=1)", -1 );
      //cmdline.addBoolean( "configHelp", 'H', "Show documentation of registered config types (on/off)", 0 );
      cmdline.addBoolean( "components", 'L', "Show component list", 0 );
      cmdline.addStr( "configHelp", 'H', "Show documentation of registered config types (on/off/argument) (if an argument is given, show only documentation for config types beginning with the name given in the argument)", NULL, 0 );
      cmdline.addStr( "configDflt", 0, "Show default config section templates for each config type (on/off/argument) (if an argument is given, show only documentation for config types beginning with the name given in the argument, OR for a list of components in conjunctions with the 'cfgFileTemplate' option enabled)", NULL, 0 );
      cmdline.addBoolean( "cfgFileTemplate", 0, "Print a complete template config file for a configuration containing the components specified in a comma separated string as argument to the 'configDflt' option", 0 );
      cmdline.addBoolean( "cfgFileDescriptions", 0, "Include description in config file templates.", 0 );
      cmdline.addBoolean( "ccmdHelp", 'c', "Show custom commandline option help (those specified in config file)", 0 );
      cmdline.addStr( "logfile", 0, "set log file", "smile.log" );
      cmdline.addBoolean( "nologfile", 1, "don't write to a log file (e.g. on a read-only filesystem)", 1 );
      cmdline.addBoolean( "noconsoleoutput", 0, "don't output any messages to the console (log file is not affected by this option)", 0 );
      cmdline.addBoolean( "appendLogfile", 0, "append log messages to an existing logfile instead of overwriting the logfile at every start", 0 );

      int help = 0;
      if (cmdline.doParse() == -1) {
        LOGGER.setLogLevel(0);
        help = 1;
      }
      if (argc <= 1) {
        printf("\nNo commandline options were given.\n Please run ' SMILExtract -h ' to see some usage information!\n\n");
        return 10;
      }

      if (help==1) { return 0; }

      if (cmdline.getBoolean("nologfile")) {
        LOGGER.setLogFile((const char *)NULL,0,!(cmdline.getBoolean("noconsoleoutput")));
      } else {
        LOGGER.setLogFile(cmdline.getStr("logfile"),cmdline.getBoolean("appendLogfile"),!(cmdline.getBoolean("noconsoleoutput")));
      }
      LOGGER.setLogLevel(cmdline.getInt("loglevel"));
      SMILE_MSG(2,"openSMILE starting!");

  #ifdef DEBUG  // ??
      if (!cmdline.getBoolean("debug"))
        LOGGER.setLogLevel(LOG_DEBUG, 0);
  #endif

      SMILE_MSG(2,"config file is: %s",cmdline.getStr("configfile"));


      // create configManager:
      configManager = new cConfigManager(&cmdline);


      cMan = new cComponentManager(configManager,componentlist);


      const char *selStr=NULL;
      if (cmdline.isSet("configHelp")) {
  #ifndef EXTERNAL_BUILD
        selStr = cmdline.getStr("configHelp");
        configManager->printTypeHelp(1/*!!! -> 1*/,selStr,0);
  #endif
        help = 1;
      }
      if (cmdline.isSet("configDflt")) {
  #ifndef EXTERNAL_BUILD
        int fullMode=0;
        int wDescr = 0;
        if (cmdline.getBoolean("cfgFileTemplate")) fullMode=1;
        if (cmdline.getBoolean("cfgFileDescriptions")) wDescr=1;
        selStr = cmdline.getStr("configDflt");
        configManager->printTypeDfltConfig(selStr,1,fullMode,wDescr);
  #endif
        help = 1;
      }
      if (cmdline.getBoolean("components")) {
  #ifndef EXTERNAL_BUILD
        cMan->printComponentList();
  #endif  // EXTERNAL_BUILD
        help = 1;
      }

      if (help==1) {
        delete configManager;
        delete cMan;
        return -1;
      }


      // TODO: read config here and print ccmdHelp...
      // add the file config reader:
      try{
        configManager->addReader( new cFileConfigReader( cmdline.getStr("configfile"), -1, &cmdline) );
        configManager->readConfig();
      } catch (cConfigException *cc) {
        return 0;
      }

      /* re-parse the command-line to include options created in the config file */
      cmdline.doParse(1,0); // warn if unknown options are detected on the commandline
      if (cmdline.getBoolean("ccmdHelp")) {
        cmdline.showUsage();
        delete configManager;
        delete cMan;
        return -1;
      }
    }catch(cSMILException *c) {
        // free exception ??
        return EXIT_ERROR;
    }
        
    return 0;
}


int extract(void)
{
    try{
//        ConfigInstance * _sourceConfig = configManager->getInstance("cWaveSource");
//        cWaveSource * source = (cWaveSource*)cMan->getComponentInstance("cWaveSource");
//        source->setFileName("abc");
        
        cMan->resetInstances();
        /* create all instances specified in the config file */
        cMan->createInstances(0); // 0 = do not read config (we already did that above..)

        /*
        MAIN TICK LOOP :
        */
        cmanGlob = cMan;
        signal(SIGINT, INThandler); // install Ctrl+C signal handler

        /* run single or mutli-threaded, depending on componentManager config in config file */
        //long long nTicks = cMan->runMultiThreaded(cmdline.getInt("nticks"));
        long long nTicks = cMan->runMultiThreaded();

        /* it is important that configManager is deleted BEFORE componentManger!
          (since component Manger unregisters plugin Dlls, which might have allocated configTypes, etc.) */
#if 0
        delete configManager;
        delete cMan;
#endif
    } catch(cSMILException *c) {
        // free exception ??
        return EXIT_ERROR;
    }

    if (ctrlc) return EXIT_CTRLC;
    return EXIT_SUCCESS;
}

//字符串分割函数
std::vector<float> mysplit(std::string str,std::string pattern)
{
    std::string::size_type pos;
    std::vector<float> result;
    str += pattern;//扩展字符串以方便操作
    int size = str.size();

    for(int i=0; i<size; i++)
    {
        pos = str.find(pattern ,i);
        if(pos < size-1)
        {
            std::string s = str.substr(i,pos-i);
            if (s.compare("'unknown'") != 0)
                result.push_back(atof(s.c_str()));
            i = pos + pattern.size() -1;
        }
    }
    return result;
}

int initOpenSMILE(const char *configPath, const char *audioDir, const char *featFilePath)
{
    int argc = 7;

    char **argv = new char *[argc];

    string exe("SMILExtract");
    argv[0] = new char [exe.length()+1]();
    strcpy(argv[0], exe.c_str());

    argv[1] = new char [3]();
    strcpy(argv[1], "-C");

    argv[2] = new char [strlen(configPath)+1]();
    strcpy(argv[2], configPath);

    argv[3] = new char [3]();
    strcpy(argv[3], "-I");
    
    g_audioSavePath.clear();
    g_audioSavePath.append(audioDir).append("/").append("sample.wav");
    argv[4] = new char [g_audioSavePath.length()+1]();
    strcpy(argv[4], g_audioSavePath.c_str());

    argv[5] = new char [3]();
    strcpy(argv[5], "-O");

    argv[6] = new char [strlen(featFilePath)+1]();
    memcpy(argv[6], featFilePath, strlen(featFilePath));
    
    int ret = 0;
    if (!is_inited_openSMILE){
        is_inited_openSMILE = true;
        ret = initOpenSMILEConfig(argc, (const char**)argv);
    }
    
    g_featFilePath = featFilePath;
    
    return ret;
}


int copyfile(const char* src, const char *dst)
{
    fstream in; //源文件读
    in.open(src, ios::in | ios::binary);
    if (!in) {
        printf("open source file error!");
        return -1;
    }
    
    int pos = 0;
    in.seekg(0, ios::end);
    long long file_size = in.tellg();
    in.seekg(0, ios::beg);
    
    FILE *wavFileFd;
    if ((wavFileFd = fopen(dst, "wb+")) == NULL)
    {
        printf("open %s failed", dst);
        return -1;
    }
    
    unsigned char *wavBody = new unsigned char[file_size+1]();
    
    in.seekg(0);
    in.read((char*)wavBody, file_size);
    
    fwrite(wavBody, 1, file_size, wavFileFd);
    
    in.close();
    fclose(wavFileFd);
    
    return 0;
}


int featExtract(const char *audioPath, vector<float> &ret)
{
    //目前不支持动态指定音频路径，这里只能将传入的音频按照固定路径复制一份
    if (g_audioSavePath.compare(audioPath))
    {
        remove(g_audioSavePath.c_str());
        copyfile(audioPath, g_audioSavePath.c_str());
    }
    //移除之前的特征文件，以免对新提取的特征产生影响
    remove(g_featFilePath.c_str());
    
    if (extract()) {
        printf("feat extract failed...\n");
        return -1;
    }
    //从csv中读取特征数据
    FILE *fp = fopen(g_featFilePath.c_str(), "r");
    if (NULL == fp)
    {
        printf("open %s failed...\n", g_featFilePath.c_str());
        return -1;
    }

    char line[LINE_SIZE] = {0};
    bool is_find_data = false;
    
    vector<float> origfeatData;
    while(!feof(fp))
    {
        fgets(line,LINE_SIZE,fp);
        string str = line;
        
        if (is_find_data && strcmp(line, "\n") && strcmp(line, "\r\n"))
        {
            origfeatData = mysplit(str, ",");
            printf("\n");
            break;
        }
        
        if (str.find("@data") != -1)
        {
            is_find_data = true;
        }
    }
    fclose(fp);
    
    //数据标准化
    int index = 0;
    if (!origfeatData.empty())
    {
        for (float data : origfeatData)
        {
            float f = (data - feat_data_mean.at(index))/feat_data_std.at(index);
            ret.push_back(f);
            index++;
        }
    }
    
    return 0;
}


void split(const char *str, vector<float> &ret)
{
    char *tokenPtr=strtok((char*)str,",");
    float f = atof(tokenPtr);
    printf("%f\n", f);
    while((tokenPtr = strtok(NULL," ")) != NULL){
        f = atof(tokenPtr);
        ret.push_back(f);
        printf("%f\n", f);
    }
}

bool readMeanAndStdFromFile(const char *filePath)
{
    FILE *fp = fopen(filePath, "r");
    if (NULL == fp)
    {
        printf("open %s failed...\n", filePath);
        return false;
    }

    char line[40000] = {0};

    //读取mean
    fgets(line, sizeof(line), fp);
    split(line, feat_data_mean);

    //读取std
    memset(line, 0, sizeof(line));
    fgets(line, sizeof(line), fp);
    split(line, feat_data_std);

    fclose(fp);

    return true;
}


#if 0
int featExtract(const char *configPath, const char *audioPath, const char *featFilePath, vector<float> &ret)
{
    int argc = 7;

    char **argv = new char *[argc];

    string exe("SMILExtract");
    argv[0] = new char [exe.length()+1]();
    strcpy(argv[0], exe.c_str());

    argv[1] = new char [3]();
    strcpy(argv[1], "-C");

    argv[2] = new char [strlen(configPath)+1]();
    strcpy(argv[2], configPath);

    argv[3] = new char [3]();
    strcpy(argv[3], "-I");

    argv[4] = new char [strlen(audioPath)+1]();
    strcpy(argv[4], audioPath);

    argv[5] = new char [3]();
    strcpy(argv[5], "-O");

    argv[6] = new char [strlen(featFilePath)+1]();
    memcpy(argv[6], featFilePath, strlen(featFilePath));
    if (!is_inited_openSMILE){
        is_inited_openSMILE = true;
        initOpenSMILEConfig(argc, (const char**)argv);
    }
    
    remove(featFilePath);
    
    extract();

    for (int i=0;i<argc;i++)
    {
        delete []argv[i];
    }

    delete []argv;
    
    FILE *fp = fopen(featFilePath, "r");
    if (NULL == fp)
    {
        printf("open %s failed...\n", featFilePath);
        return -1;
    }

    char line[LINE_SIZE] = {0};
    bool is_find_data = false;
    
    while(!feof(fp))
    {
        fgets(line,LINE_SIZE,fp);
        string str = line;
        
        if (is_find_data && strcmp(line, "\n") && strcmp(line, "\r\n"))
        {
            ret = mysplit(str, ",");
            printf("\n");
            break;
        }
        
        if (str.find("@data") != -1)
        {
            is_find_data = true;
        }
    }
    fclose(fp);
    
    return 0;
}


int featExtractTest(const char *configPath, const char *audioPath, const char *featFilePath, vector<float> &featData)
{
    vector<float> origfeatData;
#if 0
    string str = audioPath;
    int pos = str.find_last_of("/");
    string wavName = str.substr(pos+1, str.length());
    
    if (wavName.compare(sampleAudioName))
    {
        string newAudioPath = str.substr(0, pos).append("/").append(sampleAudioName);
        copyfile(audioPath, newAudioPath.c_str());
    }
#endif
    
    if (featExtract(configPath, audioPath, featFilePath, origfeatData))
    {
        printf("extract feat failed...\n");
        return -1;
    }
    
    //数据标准化
    int index = 0;
    if (!origfeatData.empty())
    {
        for (float data : origfeatData)
        {
            float f = (data - feat_data_mean.at(index))/feat_data_std.at(index);
            featData.push_back(f);
            index++;
        }
    }
    
    return 0;
}

int featExtractTest(const char *configPath, const char *audioPath, const char *featFilePath)
{
    
    vector<float> ret;
    return featExtract(configPath, audioPath, featFilePath, ret);
}
#endif
