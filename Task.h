#ifndef _TASK_H
#define _TASK_H

#include "DataDef.h"

class Task
{
public:
    void setWorkType(EnumJobType type) { m_workType = type; } // 设置任务工作类型
    EnumJobType getWorkType() { return m_workType; }    // 获取任务工作类型
    //int getJobNo(void) const { return m_jobNo; }    // 获取任务编号
    //void setJobNo(int jobNo) { m_jobNo = jobNo; }   // 设置任务编号
    //char* getJobName(void) const { return m_jobName; }  // 获取任务名称
    //void setJobName(char* jobName); // 设置任务名称
    virtual bool Run() = 0;
private:
    //int m_jobNo;
    //char* m_jobName;
    EnumJobType m_workType;
};


#endif