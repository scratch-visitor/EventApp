#ifndef __EVENT_TASK_HPP__
#define __EVENT_TASK_HPP__

#include <functional>
#include <memory>

namespace sv
{

class EventTask
{
public:
  enum EVENT_INDEX : int
  {
    event_not_defined = -1,
    event_destroy_thread = 0,
    event_do_something_once,
    event_do_something_forever,

    event_max_count
  };

public:
  EventTask();
  virtual ~EventTask();

  void start();
  void stop();

  void trigger(int _index);

  // extend method of start
  template<class...Handler>
  void start(Handler&&..._handler)
  {
    set_handler(0, std::forward<Handler>(_handler)...);
    _do_start();
  }

  // extend method of set_handler
  template<class First, class...Handler>
  void set_handler(int _index, First&& _first, Handler&&..._handler)
  {
    set_handler(_index, std::forward<First>(_first));

    set_handler(_index + 1, std::forward<Handler>(_handler)...);
  }
  template<class Last>
  void set_handler(int _index, Last&& _last)
  {
    _set_handler(_index, std::forward<Last>(_last));
  }

private:
  void _do_start();
  void _set_handler(int _index, std::function<void()> _handler);

private:
  class implement;
  std::unique_ptr<implement> impl;
};

} // namespace sv

#endif // __EVENT_TASK_HPP__