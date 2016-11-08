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

// Portable multi-way spin lock.

#ifndef MULTI_SPIN_LOCK_20170130
#define MULTI_SPIN_LOCK_20170130

// Note:  This facility can be used with other thread APIs besides std::thread.
//
#include <thread>

#include "simple_atomic.h"

struct Multi_spin_lock_default_traits
  {
    static const bool Enable_stats = false;

    using Thread_id = std::thread::id;

    static Thread_id no_thread() { return(Thread_id()); }

    static Thread_id this_tid() { return(std::this_thread::get_id());}

    static bool retry_validate(
      Simple_atomic::T<Thread_id> & /* tid */, unsigned /* tries */)
      { return(true); }
  };

// Spin lock class
//
// For all member function, "this_tid" parameter must be the thread
// id of the calling thread.
//
// The "Traits" template parameter is expected to have the following
// public members:
//
// static const bool Enable_stats -- if true, enables recording of
//   retry count high water mark.
//
// type Thread_id -- instances identify threads, must have =, ==, copy
//   constructor.
//
// static Thread_id no_thread() -- must return an instance of Thread_id
//   that will never correspond to any thread.
//
// static Thread_id this_tid() -- returns the default value for all
//   member functions with a "this_tid" parameter (which must be
//   the thread id of the thread calling the member function).  This
//   may return the same value as no_thread(), but in that case, care
//   must be taken to never omit the "this_tid" parameter.
//
// static bool
// retry_validate(Simple_atomic::T<Thread_id> &tid, unsigned tries) --
//   This function is called by the wait_lock() member function before each
//   retry at locking the spin lock.  "tries" is the number of tries (to lock)
//   preceeding the call.  It can return false to cause wait_lock() to fail.
//   It can throw an exception or exit/restart. "tid" is the private data
//   member of the spin lock.  This function could detect if the thread
//   holding the spin lock has exited.  If your environment has many threads
//   of equal priority with round-robin scheduling, it might make sense to
//   do a "yield" every time (tries % N) == 0 for some N.
//
template<class Traits = Multi_spin_lock_default_traits>
class Multi_spin_lock
  {
  public:

    using Thread_id = typename Traits::Thread_id;

    static constexpr Thread_id no_thread() { return(Traits::no_thread()); }

  private:

    // Holds the id of the thread that has locked this lock, or no_thread()
    // if it is unlocked.
    //
    Simple_atomic::T<Thread_id> tid;

    // Don't record stats by default.
    //
    // Note:  Dummy parameter is work around for prohibition by the C++
    // Standard of full template specialization in class scope.
    //
    template <bool Enable, bool Dummy = false>
    struct Stats_
      {
        static void report_retries(unsigned) { }

        static unsigned retry_high_water() { return(0); }
      };

    using Sa_uint = Simple_atomic::T<unsigned>;

    // Storage for high water mark if stats enabled.
    //
    static Sa_uint & retry_high_water_()
      {
        static Sa_uint i(Simple_atomic::No_threads);

        return(i);
      }

    // Specialization for enabled stats.
    //
    template <bool Dummy>
    struct Stats_<true, Dummy>
      {
        static unsigned retry_high_water()
          { return(retry_high_water_()); }

        static void report_retries(unsigned num)
          {
            unsigned curr = retry_high_water();

            for ( ; ; )
              {
                if (num <= curr)
                  return;

                if (retry_high_water_().compare_exchange(curr, num))
                  return;
              }
          }

        static void reset_retry_high_water()
          {
            retry_high_water_() = 0;

            Simple_atomic::make_visible();
          }

      }; // end struct Stats_<true>

    using Stats = Stats_<Traits::Enable_stats>;

    Thread_id try_lock_no_acquire_(Thread_id this_tid = Traits::this_tid())
      {
        Thread_id curr = no_thread();

        if (tid.compare_exchange(curr, this_tid))
          return(this_tid);

        return(curr);
      }

  public:

    // Construct with no_thread() if initially unlocked, otherwise with
    // locking thread if initially locked.
    //
    constexpr Multi_spin_lock(Thread_id tid_ = no_thread()) : tid(tid_) { }

    // Get retry high water mark.  (There should be an acquire fence between
    // calls to this function in the same thread).
    //
    static unsigned retry_high_water() { return(Stats::retry_high_water()); }

    // Reset retry high water mark to zero.
    //
    static void reset_retry_high_water() { Stats::reset_retry_high_water(); }

    // Try to lock without a succeeding acquire memory fence (which caller
    // must provide).  Useful if you want to implement your own retry logic
    // rather than calling wait_lock().
    //
    bool try_lock_no_acquire(Thread_id this_tid = Traits::this_tid())
      { return(try_lock_no_acquire_(this_tid) == this_tid); }

    // Try to lock once, also provides an acquire memory fence.
    // Returns false if fails to lock.
    //
    bool try_lock(Thread_id this_tid = Traits::this_tid())
      {
        Thread_id result = try_lock_no_acquire_(this_tid);

        Simple_atomic::acquire();

        return(result == this_tid);
      }

    // Try and retry to lock, repeatedly.  Returns false if fails to lock.
    // Also provides an acquire memory fence.
    //
    bool wait_lock(Thread_id this_tid = Traits::this_tid())
      {
        Thread_id try_result = try_lock_no_acquire_(this_tid);

        if (try_result == this_tid)
          {
            Simple_atomic::acquire();

            return(true);
          }

        unsigned retry_count = 0;

        for ( ; ; )
          {
            if (!Traits::retry_validate(tid, ++retry_count))
              return(false);

            try_result = try_lock_no_acquire_(this_tid);

            if (try_result == this_tid)
              {
                Stats::report_retries(retry_count);

                Simple_atomic::acquire();
                
                return(true);
              }
          }
      }

    // Unlock lock.  Should only be called by thread currently holding lock,
    // unless the holding thread has exited.
    //
    void unlock()
      {
        Simple_atomic::release();

        tid = no_thread();
      }

    // It is not safe to call this function with the parameter not
    // equal to the thread id of the thread in which it is called.
    // Aggressive optimization may mean the thread sees a stale value for
    // the 'tid' member variable, which causes this function to work
    // incorrectly when 'this_tid' in not the thread id of the calling thread.
    //
    bool is_locked_by_this_thread(
      Thread_id this_tid = Traits::this_tid()) const
      {
        return(tid == this_tid);
      }

    class Sentry
      {
      private:

        Multi_spin_lock &sl;

      public:

        Sentry(Multi_spin_lock &sl_, Thread_id tid = Traits::this_tid())
          : sl(sl_)
          { sl.wait_lock(tid); }

        ~Sentry() { sl.unlock(); }
      };

    class Nesting_sentry
      {
      private:

        Multi_spin_lock *sl_ptr;

      public:

        Nesting_sentry(Multi_spin_lock &sl, Thread_id tid = Traits::this_tid())
          : sl_ptr(&sl)
          {
            if (sl.is_locked_by_this_thread(tid))
              sl_ptr = nullptr;
            else
              sl_ptr->wait_lock();
          }

        ~Nesting_sentry()
          {
            if (sl_ptr)
              sl_ptr->unlock();
          }
      };

  }; // end class Multi_spin_lock

#endif // Include once.
