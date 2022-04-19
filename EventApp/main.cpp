#include <iostream>
#include <string>

#include "EventTask.hpp"


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
  std::shared_ptr<sv::EventTask> task = nullptr;

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
      task = std::make_shared<sv::EventTask>();
      task->set_handler(sv::EventTask::event_do_something_once,
                        [&]
                        {
                          std::cout << "hello world!\n" << std::flush;
                        });
      task->set_handler(sv::EventTask::event_do_something_forever,
                        [&]
                        {
                          std::cout << "good to see you again!\n" << std::flush;
                          if (task)
                          {
                            task->trigger(sv::EventTask::event_do_something_forever);
                          }
                        });
      task->start();
    }
    else if (line == "stop")
    {
      task = nullptr;
    }
    else if (line == "once")
    {
      if (task)
      {
        task->trigger(sv::EventTask::event_do_something_once);
      }
    }
    else if (line == "forever")
    {
      if (task)
      {
        task->trigger(sv::EventTask::event_do_something_forever);
      }
    }
    else
    {
      std::cerr << "unknown command.\n" << std::flush;
    }

  } while (true);

}