#include "HttpTask.h"

HttpTask::HttpTask()
{

}

HttpTask::HttpTask(char *url, long size)
{
    downloadUrl = url;
    MAX_SEG_SIZE = size * 1024 * 1024;
    downloadFileSize = -1;
    isSeg = false;
    fp = NULL;
}

HttpTask::~HttpTask()
{
    if (!downloadSegMap.empty()) {
        std::map<int, SubHttpTask*>::iterator it = downloadSegMap.begin();
        while (it != downloadSegMap.end()) {
            delete (*it).second;
            it++;
        }
    }
    downloadSegMap.clear();
}

bool HttpTask::Run()
{
    return true;
}

long HttpTask::GetDownloadFileSize(unsigned int *segNum)
{
    double fileLen = 0;
    CURL *handle = curl_easy_init ();
    curl_easy_setopt (handle, CURLOPT_URL, downloadUrl.c_str());
    curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt (handle, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt (handle, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt (handle, CURLOPT_HEADER, 0L);  
    curl_easy_setopt (handle, CURLOPT_NOBODY, 1L);   
    curl_easy_setopt (handle, CURLOPT_FORBID_REUSE, 1);
    //curl_easy_setopt (handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36"); //user-agent
    
    if (curl_easy_perform (handle) == CURLE_OK) {
        curl_easy_getinfo (handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fileLen);
    }
    else {
        fileLen = -1;
    }

    if (fileLen <= 0) {
        curl_easy_cleanup(handle);
        segTotal = -1;
        return segTotal;
    }

    downloadFileSize = fileLen;
    int additional = (downloadFileSize % MAX_SEG_SIZE == 0) ? 0 : 1;
    segTotal = (downloadFileSize < MAX_SEG_SIZE) ? 1 : (downloadFileSize / MAX_SEG_SIZE + additional);

    if (segTotal > 1) {
        isSeg = true;
    }

    curl_easy_cleanup(handle);
    *segNum = segTotal;
    return fileLen;
}

bool HttpTask::AssignSegTask()
{
    SubHttpTask *t = NULL;

    int additional = (downloadFileSize % MAX_SEG_SIZE == 0) ? 0 : 1;
    segTotal = (downloadFileSize < MAX_SEG_SIZE) ? 1 : (downloadFileSize / MAX_SEG_SIZE + additional);

    const char* fileName = basename((char*)downloadUrl.c_str());  //命名
    downloadFilePath  = fileName;
    downloadFilePath = "./" + downloadFilePath; 
    fp = fopen(downloadFilePath.c_str(), "wb"); 
    if(NULL == fp) {
        printf("[error] create %s file failed!\n", downloadFilePath.c_str());
        return false;
    }

    if(!isSeg){
        t = new SubHttpTask();
        t->SetParent((void*)this);
        t->startPos = 0;
        t->endPos = downloadFileSize - 1;
        t->segNo = 1;
        downloadSegMap.insert(std::map<int,SubHttpTask*>::value_type(1, t));
    }
    else{
        long segNum = downloadFileSize / MAX_SEG_SIZE;
        for(int i = 0; i <= segNum; i++) {
            t = new SubHttpTask();
            t->SetParent((void*)this);
            if(i < segNum){
                t->startPos = i * MAX_SEG_SIZE;
                t->endPos = (i + 1) * MAX_SEG_SIZE - 1;
            }
            else{
                if(downloadFileSize % MAX_SEG_SIZE != 0){
                    t->startPos = i * MAX_SEG_SIZE;
                    t->endPos = downloadFileSize - 1;
                }else
                    break;
            }
            t->segNo = i + 1;
            downloadSegMap.insert(std::map<int,SubHttpTask*>::value_type(i + 1, t));
        }
    }

    return true;
}

SubHttpTask::SubHttpTask()
{
    parent = NULL;
    segNo = -1;
    startPos = -1;
    endPos = -1;
    isFinished = false;
}

SubHttpTask::~SubHttpTask()
{

}


size_t SubHttpTask::WriteData(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    SubHttpTask* t = (SubHttpTask*) userdata;
    size_t written;

    if(t->parent->isSeg) {
        t->parent->segLocker.lock();
        if(t->startPos + size * nmemb <= t->endPos) {
            fseek(t->parent->fp, t->startPos, SEEK_SET);
            written = fwrite(ptr, size, nmemb, t->parent->fp);
            t->startPos += size * nmemb;
        }
        else {
            fseek(t->parent->fp, t->startPos, SEEK_SET);
            written = fwrite(ptr, 1, t->endPos - t->startPos + 1, t->parent->fp);
            t->startPos = t->endPos;
        }
        t->parent->segLocker.unlock();
    }
    else {
        written = fwrite(ptr, size, nmemb, t->parent->fp);
    }

    return written;
}

bool SubHttpTask::Run()
{
    CURL* curl;
    CURLcode res;
    bool bRet;

    char range[64] = { 0 };
    if(parent->isSeg) {
        snprintf(range, sizeof(range), "%ld-%ld", startPos, endPos);
    }

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, parent->downloadUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)this);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36"); 
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    //curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    if(parent->isSeg)
        curl_easy_setopt(curl, CURLOPT_RANGE, range);

    res = curl_easy_perform(curl);
    if(CURLE_OK != res){
        printf("[error] curl error:%d location:%ld segment url:%s\n", res, segNo, parent->downloadUrl.c_str());
        bRet = false;
    } else {
        isFinished = true;
        bRet = true;
    }

    curl_easy_cleanup(curl);

    return true;
}