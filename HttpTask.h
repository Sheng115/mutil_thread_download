#ifndef _HTTP_TASK_H
#define _HTTP_TASK_H

#include <libgen.h>
#include "Util.h"
#include "Task.h"
#include "Locker.h"

class SubHttpTask;

class HttpTask : public Task
{
public:
    HttpTask();
    HttpTask(char *url, long size);
    virtual ~HttpTask();

    std::string downloadUrl;    // 下载文件地址
    std::string downloadFilePath; // 下载路径
    long downloadFileSize;  // 下载文件大小
    long segTotal;  // 分段数
    long MAX_SEG_SIZE;  // 分段最大阈值,将输入的单位从MB转化成B
    bool isSeg; // 分段下载标志
    FILE *fp;   // 文件指针
    locker segLocker;   // 分段下载写文件锁
    std::map<int, SubHttpTask*> downloadSegMap;

    bool Run();
    long GetDownloadFileSize(unsigned int *segNum);
    bool AssignSegTask();
};

class SubHttpTask : public HttpTask
{
public:
    SubHttpTask();
    virtual ~SubHttpTask();

    HttpTask *parent;   // 父类指针
    long segNo; // 分段号
    long startPos;  // 分段开始位置
    long endPos;    // 分段结束位置
    bool isFinished;    // 分段下载结束标志

    bool Run();
    void SetParent(void *p) { parent = (HttpTask *)p; }

    static size_t WriteData(void *ptr, size_t size, size_t nmemb, void *userData);
};

#endif