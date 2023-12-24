#ifndef _DATADEF_H_
#define _DATADEF_H_

#define SEG_FILE_SIZE 20    // 文件开启分段下载最小阈值
#define THREAD_NUMBER 8 // 线程数
#define MAX_TASK_SIZE 99999 // 最大任务数
#define TRY_TIMES 3 // 任务数满时，可以尝试等待的次数
#define WAIT_TIME 5 // 每次等待时间（单位：秒）

typedef enum
{
    JOB_WORK_TYPE_NONE = 0,
    JOB_WORK_TYPE_HTTP,
    JOB_WORK_TYPE_FTP,
    JOB_WORK_TYPE_OTHER
}EnumJobType;

typedef struct threadpool_conf_s
{
    unsigned int thread_num;
    unsigned int thread_stack_size;
    unsigned int max_tasknum;
}threadpool_conf_t, *pthreadpool_conf_t;

#endif