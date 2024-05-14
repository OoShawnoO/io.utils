/**
  ******************************************************************************
  * @file           : TimerTask.h
  * @author         : huzhida
  * @brief          : ��ƽ̨��ʱ����ʵ��
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
    // ��ʱ����ڵ����
    // time task node base
    struct TimerTaskNodeBase {
        // Ψһ��ʶ
        // unique id
        uint32_t id;
        // ����ʱ�� ��λΪ ms
        // expire time (ms)
        int64_t expire;

        TimerTaskNodeBase(uint32_t id,int64_t expire);
    };
    // ��ʱ����ڵ�
    // timer task node
    struct TimerTaskNode : public TimerTaskNodeBase {
        using CallBack = std::function<void()>;
        // �Ƿ�ѭ��
        // recurse or not
        bool recurse;
        // �ص�����
        // callback function
        CallBack function;

        TimerTaskNode(uint32_t id,int64_t expire,bool recurse,CallBack function);
    };

    bool operator < (const TimerTaskNodeBase& task1,const TimerTaskNodeBase& task2);

    class TimerTask {
    public:
        explicit TimerTask(bool detach = true);
        ~TimerTask();
        // ��Ӷ�ʱ����
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
        // ȡ����ʱ����
        // cancel timer task
        bool CancelTask(const TimerTaskNodeBase& node);
        // ���ж�ʱ����
        // run timer task
        bool RunTimerTask();
        // ��ȡʱ��������Ķ�ʱ�����ʱ����
        // get the least time duration timer task's time duration
        time_t TimeToNextTask() const;
        // ��ȡ tick
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
