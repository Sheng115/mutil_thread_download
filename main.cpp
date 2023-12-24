#include <cstdlib>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include "Util.h"
#include "ThreadPool.h"

static void show_usage(void)
{
    printf("使用说明：\n"
                "-H http下载\n"
                "-F FTP下载\n"
                "-O 其他方式下载\n"
                "-s 分段下载阈值(单位M, 默认为20)\n"
                "-h 帮助说明"
                "\n"
                "e.g:\n"
                "mutil_thread_download -H 'url' -s 20\n"
                );
}

int main(int argc, char** argv)
{
    int ch, type = -1;  //type表明下载的方式
    char url[200] = {0};
    long seg_file_size = SEG_FILE_SIZE;

    while((ch = getopt(argc, argv, "H:F:O:s:h")) != -1)
    {
        switch(ch)
        {
            case 'H':
                strcpy(url, optarg);
                type = 1;
                break;
            case 'F':
#if FTP_DOWN
                strcpy(url, optarg);
#endif
                show_usage();
                type = 2;
                break;
            case 'O':
                show_usage();
                type = 3;
                break;
            case 's':
                seg_file_size = atol(optarg);
            case 'h':
                show_usage();
                type = 4;
                break;
            default:
                std::cout << "no args" << std::endl;
                show_usage();
                type = 5;
                break;
        }
    }

    CThreadPool<HttpTask> *httpPool = nullptr;
    HttpTask *hTask = nullptr;

#if FTP_DOWN
    //CThreadPool<FtpTask> *ftpPool = nullptr;
    //CFtpTask *fTask = nullptr;
#endif

    std::map<int, SubHttpTask*>::iterator it;

    if(type == 1)
    {
        hTask = new HttpTask(url, seg_file_size);
        unsigned int segNum = 0;
        long fileSize = hTask->GetDownloadFileSize(&segNum);
        if(fileSize <= 0)
        {
            std::cout << "[error] get file size failed! Please check url validity." << std::endl;
            goto PROC_EXIT;
        }
        hTask->setWorkType(JOB_WORK_TYPE_HTTP);

        threadpool_conf_t conf = {segNum, 0, 10000};
        try
        {
            httpPool = new CThreadPool<HttpTask>((void*) &conf);
        }
        catch(...)
        {
            std::cout << "[error] thread pool init failed!" << std::endl;
            goto PROC_EXIT;
        }

        if(!hTask->AssignSegTask())
        {
            std::cout << "[warn] assign task failed!" << std::endl;
            httpPool->SetThreadExited();
            goto PROC_EXIT;
        }
        
        if(hTask->downloadSegMap.empty())
        {
            std::cout << "[warn] assign task failed!" << std::endl;
            httpPool->SetThreadExited();
            goto PROC_EXIT;
        }

        std::cout << "[notice] downloading ..." << std::endl;
        it = hTask->downloadSegMap.begin();
        while(it != hTask->downloadSegMap.end())
        {
            httpPool->append((*it).second);
            it++;
        }

    }else if(type == 2)
    {
#if FTP_DOWN
    std::cout << "[warn] please complete FTP download method!" << std::endl;
#endif
    }else
    {
        goto PROC_END;
    }
PROC_EXIT:
    if(httpPool)
    {
        httpPool->WaitThreadsExit();
    }
    if(!hTask->downloadSegMap.empty())
    {
        it = hTask->downloadSegMap.begin();
        while(it != hTask->downloadSegMap.end())
        {
            if(!((*it).second->isFinished))
            {
                break;
            }
            it++;
        }
        if(it == hTask->downloadSegMap.end())
        {
            fclose(hTask->fp);
            printf("[notice] generate %s in the current direction\n", basename((char*)hTask->downloadFilePath.c_str()));
            printf("[notice] download sucessfully!\n");
        }else
        {
            fclose(hTask->fp);
            remove(basename((char*)hTask->downloadFilePath.c_str()));
            printf("[error] download failed!\n");
        }
    }
    if(httpPool != nullptr)
    {
        delete httpPool;
        httpPool = nullptr;
    }
    if(hTask != nullptr)
    {
        delete hTask;
        hTask = nullptr;
    }

PROC_END:
    printf("end!\n");
    return 0;
}