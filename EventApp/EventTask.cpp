#include "EventTask.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <sstream>
#include <condition_variable>
#include <iostream>

namespace sv
{

class EventTask::implement
{
  EventTask* m_parent;

  std::mutex m_mu;
  std::condition_variable m_cv;
  std::atomic<int> m_event;

  std::unordered_map<int, std::function<void()>> m_handler_map;

  std::thread m_worker;

  std::string m_error_message;
public:
  implement(EventTask* _parent)
    : m_parent(_parent)
    , m_event(event_not_defined)
  {
  }
  ~implement()
  {
    stop();
  }
  void start()
  {
    m_parent->start(std::bind(&implement::_signal_destroy_thread, this));
  }
  void stop()
  {
    _signal_destroy_thread();

    if (m_worker.joinable())
    {
      m_worker.join();
    }
  }
  void trigger(int _index)
  {
    {
      std::lock_guard<std::mutex> locker(m_mu);
      m_event.store(_index);
    }
    m_cv.notify_one();
  }
  void _set_handler(int _index, std::function<void()> _handler)
  {
    m_handler_map[_index] = _handler;
  }
  void _do_start()
  {
    m_worker = std::thread(&implement::_loop, this);
  }
  void _loop()
  {
    while (true)
    {
      int ev;

      std::unique_lock<std::mutex> locker(m_mu);
      m_cv.wait(locker, [&] { return m_event.load() != event_not_defined; });
      ev = m_event.load(); // which event is occurred?
      m_event.store(event_not_defined); // reset
      locker.unlock();

      if (ev >= event_destroy_thread &&
          ev < event_max_count)
      {
        if (ev == event_destroy_thread)
        {
          // stop thread.
          std::cout << "thread break.\n" << std::flush;
          break;
        }

        auto target = m_handler_map.find(ev);
        if (target == m_handler_map.end())
        {
          std::stringstream ss;
          ss << "Handler[" << ev << "] not found.\n";
          m_error_message = ss.str();
          return;
        }

        // call handler
        target->second();
      }
    }
  }
  void _signal_destroy_thread()
  {
    trigger(event_destroy_thread);
  }
};


EventTask::EventTask()
  : impl(new implement(this))
{}
EventTask::~EventTask()
{}
void EventTask::start()
{
  impl->start();
}
void EventTask::stop()
{
  impl->stop();
}
void EventTask::trigger(int _index)
{
  impl->trigger(_index);
}
void EventTask::_set_handler(int _index, std::function<void()> _handler)
{
  impl->_set_handler(_index, _handler);
}
void EventTask::_do_start()
{
  impl->_do_start();
}



} // namespace sv