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
        // �ӳ�ʱ�� ��λΪ ms
        // expire time (ms)
        int64_t delay;
        // �Ƿ�����ѭ��
        // recurse or not
        bool recurse;
        // �ڴ�ѭ������
        // expect recurse times
        uint16_t expect;
        // �ص�����
        // callback function
        CallBack function;

        TimerTaskNode(uint32_t id,int64_t expire,int64_t delay,bool recurse,uint16_t expect,CallBack function);
    };

    bool operator < (const TimerTaskNodeBase& task1,const TimerTaskNodeBase& task2);

    class TimerTask {
    public:
        explicit TimerTask(bool detach = true);
        ~TimerTask();
        // ��Ӷ�ʱ����
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
        // ��Ӷ�ʱ����
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
        // ȡ����ʱ����
        // cancel timer task
        bool CancelTask(uint32_t id);
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
