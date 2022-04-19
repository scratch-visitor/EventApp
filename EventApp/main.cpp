// system header files
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>

// STL header files
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <condition_variable>
#include <mutex>
#include <array>
#include <bitset>

class Task
{
public:
  enum EVENT_INDEX : int
  {
    event_not_defined = -1,
    event_destroy_thread = 0,
    event_do_something,

    event_max_count
  };
public:
  Task()
    : m_event(event_not_defined)
  {
  }
  ~Task()
  {
    stop();
  }
  template<class...Handler>
  void start(Handler&&...handler)
  {
    set_handler(0, std::forward<Handler>(handler)...);

    m_worker = std::thread(&Task::_loop, this);
  }
  void start()
  {
    start(std::bind(&Task::_signal_destroy_thread, this));
  }
  void stop()
  {
    _signal_destroy_thread();

    if (m_worker.joinable())
    {
      m_worker.join();
    }
  }
  void trigger(int index)
  {
    {
      std::lock_guard<std::mutex> locker(m_mu);
      m_event.store(index);
    }
    m_cv.notify_one();
  }
  template<class First, class...Handler>
  void set_handler(int _index, First&& first, Handler&&...handler)
  {
    m_handler_map[_index] = std::forward<First>(first);

    set_handler(_index + 1, std::forward<Handler>(handler)...);
  }
  template<class Last>
  void set_handler(int _index, Last&& last)
  {
    m_handler_map[_index] = last;
  }
private :
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

private:
  std::mutex m_mu;
  std::condition_variable m_cv;
  std::atomic<int> m_event;

  std::unordered_map<int, std::function<void()>> m_handler_map;

  std::thread m_worker;

  std::string m_error_message;
};

void prompt();
int main()
{
  try
  {
    prompt();
  }
  catch (std::exception& ex)
  {
    std::cerr << "exception: " << ex.what() << std::endl;
  }

  return 0;
}

void prompt()
{
  std::shared_ptr<Task> task = nullptr;

  do
  {
    std::string line;
    do
    {
      std::cout << "PS> ";
      std::getline(std::cin, line);
    } while (line.empty());

    if (line == "quit" || line == "exit")
    {
      std::cout << "terminating program...";
      break;
    }
    else if (line == "start")
    {
      task = std::make_shared<Task>();
      task->set_handler(Task::event_do_something,
                        [&]
                        {
                          std::cout << "hello world!\n" << std::flush;
                          if (task)
                          {
                            task->trigger(Task::event_do_something);
                          }
                        });
      task->start();
    }
    else if (line == "stop")
    {
      task = nullptr;
    }
    else if (line == "event")
    {
      if (task)
      {
        task->trigger(Task::event_do_something);
      }
    }
    else
    {
      std::cerr << "unknown command.\n" << std::flush;
    }

  } while (true);

}