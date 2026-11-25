#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

/*
Lookup table of IPv4 address from host-name. All public member functions are thread safe. The hash table is
typically expected to have fewer than 4096 entries.
*/
class Host_addr_tbl
  {
  private:
    struct Node
      {
        std::string host_name;
        std::uint32_t ipv4_addr;
        std::shared_ptr<Node const> next;

        Node(std::string const &name, std::uint32_t addr, std::shared_ptr<Node const> next_node)
          : host_name(name), ipv4_addr(addr), next(std::move(next_node))
          {
          }
      };

    static constexpr std::size_t bucket_count = 8192;
    using bucket_type = std::shared_ptr<Node const>;

    static std::size_t bucket_index(std::string const &host_name)
      {
        return std::hash<std::string>{}(host_name) % bucket_count;
      }

  public:

    // Insert new host name. If host name already in table, returns false and does nothing.
    //
    static bool insert(std::string const &host_name, std::uint32_t ipv4_addr);

    // Delete host name. If host name not in table, returns false and does nothing.
    //
    static bool remove(std::string const &host_name);

    // Lookup host name and return corresponding IPv4 address. Returns zero if host name not in table. Calling
    // this will never block the thread. Return value may not reflect any in-progress insert or delete in another
    // thread. The implementation is optimized to minimize the latency of this function call.
    //
    static std::uint32_t lookup(std::string const &host_name);

    // This class is for scoping only, no instances.
    //
    Host_addr_tbl() = delete;

  private:
    // Each bucket is an immutable linked chain. Readers atomically load a bucket without taking a mutex;
    // writers serialize only with other writers to the same bucket.
    inline static std::array<bucket_type, bucket_count> buckets_{};
    inline static std::array<std::mutex, bucket_count> bucket_mutexes_{};
  };

inline bool Host_addr_tbl::insert(std::string const &host_name, std::uint32_t ipv4_addr)
  {
    auto const index = bucket_index(host_name);
    std::lock_guard<std::mutex> lock(bucket_mutexes_[index]);
    auto head = std::atomic_load_explicit(&buckets_[index], std::memory_order_acquire);

    for (auto node = head; node; node = node->next)
      if (node->host_name == host_name)
        return false;

    auto updated = std::make_shared<Node>(host_name, ipv4_addr, std::move(head));
    std::atomic_store_explicit(
      &buckets_[index], bucket_type(std::move(updated)), std::memory_order_release);
    return true;
  }

inline bool Host_addr_tbl::remove(std::string const &host_name)
  {
    auto const index = bucket_index(host_name);
    std::lock_guard<std::mutex> lock(bucket_mutexes_[index]);
    auto head = std::atomic_load_explicit(&buckets_[index], std::memory_order_acquire);

    std::vector<Node const *> prefix;
    auto node = head;
    while (node && node->host_name != host_name)
      {
        prefix.push_back(node.get());
        node = node->next;
      }
    if (!node)
      return false;

    // Rebuild only the portion of this bucket before the removed node, sharing the remaining tail.
    bucket_type updated = node->next;
    for (auto iter = prefix.rbegin(); iter != prefix.rend(); ++iter)
      updated = std::make_shared<Node>((*iter)->host_name, (*iter)->ipv4_addr, std::move(updated));

    std::atomic_store_explicit(&buckets_[index], std::move(updated), std::memory_order_release);
    return true;
  }

inline std::uint32_t Host_addr_tbl::lookup(std::string const &host_name)
  {
    auto const index = bucket_index(host_name);
    auto node = std::atomic_load_explicit(&buckets_[index], std::memory_order_acquire);
    for (; node; node = node->next)
      if (node->host_name == host_name)
        return node->ipv4_addr;
    return 0;
  }
