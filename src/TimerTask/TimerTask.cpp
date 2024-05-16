/**
  ******************************************************************************
  * @file           : TimerTask.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/5/14
  ******************************************************************************
  */

#include "TimerTask.h"

#include <utility>

namespace hzd {

    const std::string io_timer_task_channel = "io.TimerTask";

    TimerTaskNodeBase::TimerTaskNodeBase(uint32_t _id, int64_t _expire)
    : id(_id),expire(_expire){}

    TimerTaskNode::TimerTaskNode(uint32_t _id,int64_t _expire, int64_t _delay, bool _recurse,uint16_t _expect,TimerTaskNode::CallBack _function)
    : TimerTaskNodeBase(_id,_expire),delay(_delay),recurse(_recurse),expect(_expect),function(std::move(_function)){}

    bool operator < (const TimerTaskNodeBase& task1,const TimerTaskNodeBase& task2) {
        if(task1.expire > task2.expire) {
            return false;
        }else if(task1.expire < task2.expire) {
            return true;
        }else{
            return task1.id < task2.id;
        }
    }

    uint32_t TimerTask::timer_gid = 1;

    TimerTask::TimerTask(bool detach) {
        if(detach) {
            timer_mutex = std::make_unique<std::mutex>();
            timer_semaphore = std::make_unique<Semaphore>(0);
            timer_condition_variable = std::make_unique<std::condition_variable>();
            timer_thread = std::make_unique<std::thread>(DetachedLoop,this);
        }
    }

    void TimerTask::AddTask(const TimerTaskNode& task) {
        if(timer_mutex){
            std::unique_lock<std::mutex> guard(*timer_mutex);
            task_id_map[task.id] = &(*tasks.insert({
                  task.id,
                  GetTicks() + task.delay,
                  task.delay,
                  task.recurse,
                  static_cast<uint16_t>(task.expect-1),
                  task.function
            }).first);
            guard.unlock();
            timer_condition_variable->notify_all();
            timer_semaphore->Signal();
            return;
        }
        task_id_map[task.id] = &(*tasks.insert({
               task.id,
               GetTicks() + task.delay,
               task.delay,
               task.recurse,
               static_cast<uint16_t>(task.expect-1),
               task.function
        }).first);
    };


    bool TimerTask::CancelTask(uint32_t id) {
        if(timer_mutex) {
            std::unique_lock<std::mutex> guard(*timer_mutex);
            auto id_iter = task_id_map.find(id);
            if(id_iter == task_id_map.end()) {
                MOLE_WARN(io_timer_task_channel,"no such task or timer task had already expired");
                return false;
            }
            auto iter = tasks.find(*id_iter->second);
            if(iter == tasks.end()) {
                MOLE_WARN(io_timer_task_channel,"no such task or timer task had already expired");
                return false;
            }
            tasks.erase(iter);
            task_id_map.erase(id_iter);
            guard.unlock();
            timer_condition_variable->notify_all();
            return true;
        }
        auto id_iter = task_id_map.find(id);
        if(id_iter == task_id_map.end()) {
            MOLE_WARN(io_timer_task_channel,"no such task or timer task had already expired");
            return false;
        }
        auto iter = tasks.find(*id_iter->second);
        if(iter == tasks.end()) {
            MOLE_WARN(io_timer_task_channel,"no such task or timer task had already expired");
            return false;
        }
        tasks.erase(iter);
        task_id_map.erase(id_iter);
        return true;
    }

    bool TimerTask::RunTimerTask() {
        if(timer_mutex){
            std::unique_lock<std::mutex> guard(*timer_mutex);
            auto iter = tasks.begin();
            if(tasks.end() != iter && iter->expire <= GetTicks()) {
                iter->function();
                task_id_map.erase(iter->id);
                if(iter->recurse || iter->expect > 0) {
                    guard.unlock();
                    AddTask(*iter);
                    guard.lock();
                }
                tasks.erase(iter);
                return true;
            }
            return false;
        }
        auto iter = tasks.begin();
        if(tasks.end() != iter && iter->expire <= GetTicks()) {
            iter->function();
            task_id_map.erase(iter->id);
            if(iter->recurse || iter->expect > 0) AddTask(*iter);
            tasks.erase(iter);
            return true;
        }
        return false;
    }

    time_t TimerTask::TimeToNextTask() const {
        if(timer_mutex) {
            std::unique_lock<std::mutex> guard(*timer_mutex);
            auto iter = tasks.begin();
            if(iter == tasks.end()) return -1;
            time_t duration = iter->expire - GetTicks();
            return duration > 0 ? duration : 0;
        }
        auto iter = tasks.begin();
        if(iter == tasks.end()) return -1;
        time_t duration = iter->expire - GetTicks();
        return duration > 0 ? duration : 0;
    }

    time_t TimerTask::GetTicks() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch()
        ).count();
    }

    void TimerTask::DetachedLoop(TimerTask *timer_task_object) {
        TimerTask& timer_task = *timer_task_object;
        while(!timer_task.timer_stop) {
            timer_task.timer_semaphore->Wait();
    REIN:
            auto time_to_sleep = timer_task.TimeToNextTask();
            time_to_sleep = time_to_sleep > 0 ? time_to_sleep : 0;
            {
                std::unique_lock<std::mutex> timer_unique_lock(*timer_task.timer_mutex);
                if (timer_task.timer_condition_variable->wait_for(
                        timer_unique_lock,
                        std::chrono::milliseconds(time_to_sleep)
                ) != std::cv_status::timeout) {
                    goto REIN;
                }
            }
            while(timer_task.RunTimerTask());
        }
    }

    TimerTask::~TimerTask() {
        if(timer_thread) {
            {
                std::unique_lock<std::mutex> guard(*timer_mutex);
                timer_stop = true;
            }
            timer_semaphore->SignalAll();
            timer_thread->join();
        }
    }
} // hzd