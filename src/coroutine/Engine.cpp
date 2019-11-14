#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
namespace Afina {
namespace Coroutine {


ATTRIBUTE_NO_SANITIZE_ADDRESS
void Engine::Store(context &ctx)
{
  char s_end[1];
  {
    //high addr StackBottom
    //
    //
    //low addr  v

    ctx.Hight = StackBottom;
    ctx.Low = s_end;

    std::cout <<"store:" <<  (int64_t)ctx.Hight <<" " <<(int64_t)ctx.Low   <<std::endl;
    uint32_t &old_size = std::get<1>(ctx.Stack);
    char * &buffer = std::get<0>(ctx.Stack);
    auto new_size = ctx.Hight - ctx.Low;

    if (old_size < new_size || buffer==nullptr)
    {
        //not enough space for stack -> increase size
        if(buffer!=nullptr)
            delete[] buffer;
        buffer = new char[new_size];
        old_size = new_size;
    }

    memcpy(buffer, ctx.Low, new_size);
  }
}
ATTRIBUTE_NO_SANITIZE_ADDRESS
void Engine::Restore(context &ctx)
{

    char s_end[1];
    {
      std::cout <<"REstore:" << (uint64_t)s_end <<std::endl;
      if (s_end >= ctx.Low)
      {
          std::cout <<"Restore recursive call:" << (uint64_t)ctx.Hight <<std::endl;
          Restore(ctx);
      }
      size_t size = ctx.Hight - ctx.Low;
      char* &buffer = std::get<0>(ctx.Stack);


      memcpy(ctx.Low, buffer, size);
      longjmp(ctx.Environment, 1);
    }
}
ATTRIBUTE_NO_SANITIZE_ADDRESS
void Engine::yield()
{
  std::cout <<"Yield:" <<(uint64_t)cur_routine <<std::endl;
  auto c = alive;
  if(cur_routine!=nullptr)
  {

    if(alive==cur_routine)
    {
       c = alive->next;
    }

  }
  if(c!=nullptr)
  {
    Switch(*c);
  }

}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void Engine::sched(void *routine)
{
  std::cout <<"Sched:" <<(uint64_t)cur_routine <<" "  <<(uint64_t)routine <<std::endl;
  if (cur_routine == routine)
  {
    return;
  }

  if(routine==nullptr)
  {
    yield();
  }
  else
  {
    Switch(*(reinterpret_cast<context *>(routine)));
  }

}

ATTRIBUTE_NO_SANITIZE_ADDRESS
void Engine::Switch(context& ctx)
{
    std::cout <<"Sched:" <<(uint64_t)cur_routine <<" "  <<(uint64_t)&ctx <<std::endl;
    if(cur_routine && cur_routine != idle_ctx)
    {
        if (setjmp(cur_routine->Environment) > 0)
        {
            //we successfully switched to another coroutine
            //and then returned to this coroutine
            return;
        }
        else
        {
          //save state
          Store(*cur_routine);
          //change curerent pointer and enter ctx
          cur_routine = &ctx;
          Restore(ctx);
        }

    }
    else
    {
      cur_routine = &ctx;
      Restore(ctx);
    }

}

} // namespace Coroutine
} // namespace Afina
