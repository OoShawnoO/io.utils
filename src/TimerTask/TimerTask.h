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
        // 延迟时间 单位为 ms
        // expire time (ms)
        int64_t delay;
        // 是否永久循环
        // recurse or not
        bool recurse;
        // 期待循环次数
        // expect recurse times
        uint16_t expect;
        // 回调函数
        // callback function
        CallBack function;

        TimerTaskNode(uint32_t id,int64_t expire,int64_t delay,bool recurse,uint16_t expect,CallBack function);
    };

    bool operator < (const TimerTaskNodeBase& task1,const TimerTaskNodeBase& task2);

    class TimerTask {
    public:
        explicit TimerTask(bool detach = true);
        ~TimerTask();
        // 添加定时任务
        // add timer task
        template<class Rep,class Period,class Func,class... Args>
        uint32_t AddTask(
                const std::chrono::duration<Rep,Period>&    delay,
                bool                                        is_recurse,
                Func&&                                      func,
                Args&&...                                   args
        ) {
            if(timer_mutex){
                auto function = [func,args...] { func(args...); };
                std::unique_lock<std::mutex> guard(*timer_mutex);
                long _delay = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
                task_id_map[timer_gid] = &(*tasks.insert(
                        TimerTaskNode{
                                timer_gid,
                                GetTicks() + _delay,
                                _delay,
                                is_recurse,
                                0,
                                std::move(function)
                        }
                ).first);
                timer_condition_variable->notify_all();
                timer_semaphore->Signal();
                return timer_gid++;
            }
            auto function = [func,args...] { func(args...); };
            long _delay = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
            task_id_map[timer_gid] = &(*tasks.insert(
                    TimerTaskNode{
                            timer_gid,
                            GetTicks() + _delay,
                            _delay,
                            is_recurse,
                            0,
                            std::move(function)
                    }
            ).first);
            return timer_gid++;
        };
        // 添加定时任务
        // add timer task
        template<class Rep,class Period,class Func,class... Args>
        uint32_t AddTimesTask(
                const std::chrono::duration<Rep,Period>&    delay,
                uint16_t                                    expect_times,
                Func&&                                      func,
                Args&&...                                   args
        ) {
            if(expect_times <= 0) return 0;
            if(timer_mutex){
                auto function = [func,args...] { func(args...); };
                std::unique_lock<std::mutex> guard(*timer_mutex);
                long _delay = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
                task_id_map[timer_gid] = &(*tasks.insert(
                        TimerTaskNode{
                                timer_gid,
                                GetTicks() + _delay,
                                _delay,
                                false,
                                expect_times,
                                std::move(function)
                        }
                ).first);
                timer_condition_variable->notify_all();
                timer_semaphore->Signal();
                return timer_gid++;
            }
            auto function = [func,args...] { func(args...); };
            long _delay = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
            task_id_map[timer_gid] = &(*tasks.insert(
                    TimerTaskNode{
                            timer_gid,
                            GetTicks() + _delay,
                            _delay,
                            false,
                            expect_times,
                            std::move(function)
                    }
            ).first);
            return timer_gid++;
        }
        // 取消定时任务
        // cancel timer task
        bool CancelTask(uint32_t id);
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
        void AddTask(const TimerTaskNode& task);

        static uint32_t                                         timer_gid;
        std::set<TimerTaskNode,std::less<>>                     tasks;
        std::unordered_map<uint32_t,const TimerTaskNodeBase*>   task_id_map;
        bool                                                    timer_stop = false;
        std::unique_ptr<Semaphore>                              timer_semaphore = nullptr;
        std::unique_ptr<std::condition_variable>                timer_condition_variable = nullptr;
        std::unique_ptr<std::mutex>                             timer_mutex = nullptr;
        std::unique_ptr<std::thread>                            timer_thread = nullptr;
    };

} // hzd

#endif //IO_UTILS_TIMERTASK_H
