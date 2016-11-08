#include <atomic>
#include <bitset>
#include <thread>
#include <cinttypes>
#include <iostream>

constexpr unsigned Num_threads = 16;
constexpr unsigned Num_count = 4 * 1024 * 1024;

bool failed;

template <typename Integral, Integral (*incr_func)(std::atomic<Integral> &)>
class Test
{
public:
  Test()
  {
    for (unsigned idx = 1; idx < Num_threads; ++idx)
      th[idx] = std::thread(thread_func, &i, did + idx, &starting_pistol);

    starting_pistol = true;

    // Main thread is thread index 0.
    //
    thread_func(&i, did, &starting_pistol);

    for (unsigned idx = 1; idx < Num_threads; ++idx)
      th[idx].join();

    std::bitset<Num_count> anded, all;

    for (unsigned idx = 0; idx < Num_threads; ++idx)
    {
      anded = all bitand did[idx];

      if (anded.any())
      {
        failed = true;
        for (unsigned bidx = 0; bidx < Num_count; ++bidx)
          if (anded[bidx])
          {
            std::cout << "FAILED: double set: thread_index=" << idx
                      << " count=" << (bidx + 1) << '\n';
            return;
          }
      }
      all = all bitor did[idx];
    }
    if (not all.all())
    {
      failed = true;
      for (unsigned bidx = 0; bidx < Num_count; ++bidx)
        if (not all[bidx])
        {
          std::cout << "FAILED: not set: count: " << (bidx + 1) << '\n';
          return;
        }
    }
    for (unsigned idx = 0; idx < Num_threads; ++idx)
      std::cout << "count[" << idx << "]=" << did[idx].count() << '\n';
  }

private:
  static void thread_func(
    std::atomic<Integral> *i, std::bitset<Num_count> *did,
    std::atomic<bool> *starting_pistol)
  {
    Integral ii;

    while (not *starting_pistol)
      ;

    do
    {
      ii = incr_func(*i);
      if (ii <= Num_count)
      {
        (*did)[ii - 1] = true;
      }
    }
    while (ii < Num_count);
  }

  std::atomic<Integral> i{0};
  std::thread th[Num_threads];
  std::bitset<Num_count> did[Num_threads];
  std::atomic<bool> starting_pistol{false};
};

template <typename Integral>
Integral incr_ce_weak(std::atomic<Integral> &i)
{
  Integral curr = i;

  while (not i.compare_exchange_weak(curr, curr + 1))
    ;

  return(curr + 1);
}

template <typename Integral>
Integral incr_ce_strong(std::atomic<Integral> &i)
{
  Integral curr = i;

  while (not i.compare_exchange_strong(curr, curr + 1))
    ;

  return(curr + 1);
}

template <typename Integral>
Integral incr_op(std::atomic<Integral> &i)
{
  return ++i;
}

template <typename Integral>
Integral incr_fa(std::atomic<Integral> &i)
{
  return i.fetch_add(1) + 1;
}

template <typename Integral>
void test_for_type()
{
  std::cout << "\nincr_ce_weak\n";
  Test<Integral, incr_ce_weak<Integral> >();

  std::cout << "\nincr_ce_strong\n";
  Test<Integral, incr_ce_strong<Integral> >();

  std::cout << "\nincr_op\n";
  Test<Integral, incr_op<Integral> >();

  std::cout << "\nincr_fa\n";
  Test<Integral, incr_fa<Integral> >();
}

int main()
{
  #if 0
  std::cout << "\nuint16\n";
  test_for_type<std::uint16_t>();
  #endif

  std::cout << "\nuint32\n";
  test_for_type<std::uint32_t>();

  std::cout << "\nuint64\n";
  test_for_type<std::uint64_t>();

  std::cout << "\nuintptr\n";
  test_for_type<std::uintptr_t>();

  if (not failed)
    std::cout << "\nSUCCESS\n";

  return(0);
}
