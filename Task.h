#ifndef _TASK_H
#define _TASK_H

#include "DataDef.h"

class Task
{
public:
    void setWorkType(EnumJobType type) { m_workType = type; } // 设置任务工作类型
    EnumJobType getWorkType() { return m_workType; }    // 获取任务工作类型
    virtual bool Run() = 0; // 任务执行函数
private:
    EnumJobType m_workType;
};


#endif