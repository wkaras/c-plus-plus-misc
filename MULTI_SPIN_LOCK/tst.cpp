/*
Copyright (c) 2017 Walter William Karas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Unit testing for multi_spin_lock.h.

#include "multi_spin_lock.h"
#include "multi_spin_lock.h" // test re-inclusion guard

#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

struct Traits : public Multi_spin_lock_default_traits
  {
    static const bool Enable_stats = true;
  };

using Spin_lock = Multi_spin_lock<Traits>;

Spin_lock sl;

unsigned i = 0, j = 1;

bool done;

// thread_lock_count[i] will hold the number of times the thread with index
// i locked the spin lock "sl".
//
std::vector<unsigned> thread_lock_count;

class Test_thread
  {
  public:
    Test_thread() : index(count++) { }

    void operator () ()
      {
        static unsigned trace_val = 1000;
        int random;

        while (!done)
          {
            {
              Spin_lock::Sentry sentry(sl);

              ++thread_lock_count[index];

              if (i == trace_val)
                {
                  std::cout << "i = " << i << std::endl;

                  trace_val *= 10;
                }

              if (j != (i + 1))
                std::cout << std::this_thread::get_id() << ' ' << i << ' ' << j
                          << std::endl;

              ++i;

              if ((thread_lock_count[index] bitand 0x7ffff) == 0)
                  std::this_thread::sleep_for(std::chrono::milliseconds(1));

              ++j;

              random = std::rand() bitand ((1 << 15) - 1);
            }

            if (random >= (3 * (1 << 12)))
              ; // Immediately try to relock 25% of the time
            else if (random >= (2 * (1 << 12)))
              // Sleep for 0 - 31 milliseconds.
              std::this_thread::sleep_for(std::chrono::milliseconds(
                (random >> 7) - 64));
            else
              while (random--)
                ;
          }
      }

  private:

    unsigned index;

    static unsigned count;
  };

unsigned Test_thread::count;

int main(int n_arg, const char * const *arg)
  {
    int num_threads, seed;

    if ((n_arg < 2) or ((num_threads = std::atoi(arg[1])) < 1) or
        (n_arg > 3) or ((seed = std::atoi(arg[1])) < 0))
      {
        std::cerr << "requires one parameter: number of threads\n";
        std::cerr << "optional second parameter: random seed (positive)\n";

        std::exit(1);
      }

    std::srand(unsigned(seed));

    std::vector<std::thread> t;

    thread_lock_count.resize(num_threads);

    for (int i = 0; i < num_threads; ++i)
      t.emplace_back(Test_thread());

    std::cout << "Hit enter to stop:" << std::endl;

    char dummy;

    std::cin.get(dummy);

    done = true;

    for (int i = 0; i < num_threads; ++i)
      t[i].join();

    std::cout << "\nFinal retry high water = " << Spin_lock::retry_high_water()
              << "\n\n";

    {
      if (sl.is_locked_by_this_thread())
        std::cout << "Should not be locked\n";

      Spin_lock::Nesting_sentry nesting_sentry1(sl);

      {
        if (!sl.is_locked_by_this_thread())
          std::cout << "Should be locked\n";

        Spin_lock::Nesting_sentry nesting_sentry2(sl);

        if (!sl.is_locked_by_this_thread())
          std::cout << "Should be locked\n";
      }

      if (!sl.is_locked_by_this_thread())
        std::cout << "Should be locked\n";
    }
    if (sl.is_locked_by_this_thread())
      std::cout << "Should not be locked\n";

    unsigned ttl = 0, max = 0, min = ~unsigned(0);

    for (int i = 0; i < num_threads; ++i)
      {
        if (thread_lock_count[i] > max)
          max = thread_lock_count[i];

        if (thread_lock_count[i] < min)
          min = thread_lock_count[i];

        ttl += thread_lock_count[i];
      }

    std::cout << "lock counts: max = " << max << ", min = " << min 
              << ", average = " << ((ttl + (num_threads / 2)) / num_threads)
              << '\n';

    return(0);
  }
