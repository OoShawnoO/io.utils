/**
  ******************************************************************************
  * @file           : TimerTask.h
  * @author         : huzhida
  * @brief          : 跨平台定时任务实现
  * @date           : 2024/5/14
  ******************************************************************************
  */

#ifndef IO_UTILS_TIMERTASK_H
#define IO_UTILS_TIMERTASK_H

#include "Mole.h"
#include <mutex>
#include <set>
#include <functional>

namespace hzd {
    // 定时任务节点基类
    // time task node base
    struct TimerTaskNodeBase {
        // 唯一标识
        // unique id
        uint32_t id;
        // 过期时间 单位为 ms
        // expire time (ms)
        int64_t expire;

        TimerTaskNodeBase(uint32_t id,int64_t expire);
    };
    // 定时任务节点
    // timer task node
    struct TimerTaskNode : public TimerTaskNodeBase {
        using CallBack = std::function<void()>;
        // 是否循环
        // recurse or not
        bool recurse;
        // 回调函数
        // callback function
        CallBack function;

        TimerTaskNode(uint32_t id,int64_t expire,bool recurse,CallBack function);
    };

    bool operator < (const TimerTaskNodeBase& task1,const TimerTaskNodeBase& task2);

    class TimerTask {
    public:
        explicit TimerTask(bool detach = true);
        ~TimerTask();
        // 添加定时任务
        // add timer task
        template<class Func,class... Args>
        TimerTaskNodeBase AddTask(
                int64_t                     expire,
                bool                        is_recurse,
                Func&&                      func,
                Args&&...                   args
        ) {
            if(timer_mutex){
                std::lock_guard<std::mutex> guard(*timer_mutex);
                auto function = [func,args...] { func(args...); };
                auto& result = *tasks.insert(
                        TimerTaskNode{
                                timer_gid++,
                                GetTicks() + expire,
                                is_recurse,
                                std::move(function)
                        }
                ).first;
                timer_semaphore->Signal();
                return result;
            }
            auto function = [func,args...] { func(args...); };
            return *tasks.insert(
                    TimerTaskNode{
                            timer_gid++,
                            GetTicks() + expire,
                            is_recurse,
                            std::move(function)
                    }
            ).first;
        };
        // 取消定时任务
        // cancel timer task
        bool CancelTask(const TimerTaskNodeBase& node);
        // 运行定时任务
        // run timer task
        bool RunTimerTask();
        // 获取时间间隔最近的定时任务的时间间隔
        // get the least time duration timer task's time duration
        time_t TimeToNextTask() const;
        // 获取 tick
        // get tick
        static time_t GetTicks();
    private:
        static void DetachedLoop(TimerTask* timer_task_object);

        static uint32_t                                         timer_gid;
        std::set<TimerTaskNode,std::less<>>                     tasks;
        bool                                                    timer_stop = false;
        std::unique_ptr<Semaphore>                              timer_semaphore = nullptr;
        std::unique_ptr<std::mutex>                             timer_mutex = nullptr;
        std::unique_ptr<std::thread>                            timer_thread = nullptr;
    };

} // hzd

#endif //IO_UTILS_TIMERTASK_H
