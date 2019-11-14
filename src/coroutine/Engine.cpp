#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {



void Engine::Store(context &ctx)
{
  char v(0);
//  {
    //high addr StackBottom
    //
    //
    //low addr  v

    ctx.Hight = StackBottom;
    ctx.Low = &v;


    uint32_t &old_size = std::get<1>(ctx.Stack);
    char * &buffer = std::get<0>(ctx.Stack);
    auto new_size = ctx.Hight - ctx.Low;

    if (old_size < new_size || buffer==nullptr)
    {
        //not enough space for stack -> increase size
        delete[] buffer;
        buffer = new char[new_size];
        old_size = new_size;
    }

    memcpy(buffer, ctx.Low, new_size);
//  }
}
void Engine::Restore(context &ctx)
{
    char s_end[2];
    {

      if (s_end >= ctx.Low && s_end <= ctx.Hight)
      {
          Restore(ctx);
      }
      size_t size = ctx.Hight - ctx.Low;
      char* &buffer = std::get<0>(ctx.Stack);


      memcpy(ctx.Low, buffer, size);
      longjmp(ctx.Environment, 1);
    }
}
void Engine::yield()
{

  auto c = alive;
  if(alive==cur_routine)
  {
     c = alive->next;
  }
  if(c!=nullptr)
  {
    Switch(*c);
  }

}
void Engine::sched(void *routine)
{
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


void Engine::Switch(context& ctx)
{
    if(cur_routine && cur_routine != idle_ctx)
    {
        if (setjmp(cur_routine->Environment) > 0)
        {
            //we successfully switched to another coroutine
            //and then returned to this coroutine
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

}

} // namespace Coroutine
} // namespace Afina
