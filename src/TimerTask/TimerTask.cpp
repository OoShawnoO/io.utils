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
    TimerTaskNodeBase::TimerTaskNodeBase(uint32_t _id, int64_t _expire)
    : id(_id),expire(_expire){}

    TimerTaskNode::TimerTaskNode(uint32_t _id, int64_t _expire, bool _recurse, TimerTaskNode::CallBack _function)
    : TimerTaskNodeBase(_id,_expire),recurse(_recurse),function(std::move(_function)){}

    bool operator < (const TimerTaskNodeBase& task1,const TimerTaskNodeBase& task2) {
        if(task1.expire > task2.expire) {
            return false;
        }else if(task1.expire < task2.expire) {
            return true;
        }else{
            return task1.id < task2.id;
        }
    }

    uint32_t TimerTask::timer_gid = 0;

    TimerTask::TimerTask(bool detach) {
        if(detach) {
            timer_mutex = std::make_unique<std::mutex>();
            timer_semaphore = std::make_unique<Semaphore>(0);
            timer_thread = std::make_unique<std::thread>(DetachedLoop,this);
        }
    }

    bool TimerTask::CancelTask(const TimerTaskNodeBase &node) {
        if(timer_mutex) {
            std::lock_guard<std::mutex> guard(*timer_mutex);
            auto iter = tasks.find(node);
            if(iter == tasks.end()) return false;
            tasks.erase(iter);
            return true;
        }
        auto iter = tasks.find(node);
        if(iter == tasks.end()) return false;
        tasks.erase(iter);
        return true;
    }

    bool TimerTask::RunTimerTask() {
        if(timer_mutex){
            std::lock_guard<std::mutex> guard(*timer_mutex);
            auto iter = tasks.begin();
            if(tasks.end() != iter && iter->expire <= GetTicks()) {
                iter->function();
                tasks.erase(iter);
                return true;
            }
            return false;
        }
        auto iter = tasks.begin();
        if(tasks.end() != iter && iter->expire <= GetTicks()) {
            iter->function();
            tasks.erase(iter);
            return true;
        }
        return false;
    }

    time_t TimerTask::TimeToNextTask() const {
        if(timer_mutex) {
            std::lock_guard<std::mutex> guard(*timer_mutex);
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
            auto time_to_sleep = timer_task.TimeToNextTask();
            time_to_sleep = time_to_sleep > 0 ? time_to_sleep : 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(time_to_sleep));
            timer_task.RunTimerTask();
        }
    }

    TimerTask::~TimerTask() {
        if(timer_thread) {
            timer_stop = true;
            timer_semaphore->SignalAll();
            timer_thread->join();
        }
    }

} // hzd