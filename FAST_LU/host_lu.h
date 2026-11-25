#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

/*
Lookup table of IPv4 address from host-name. All public member functions are thread safe. The hash table is
typically expected to have fewer than 4096 entries.
*/
class Host_addr_tbl
  {
  private:
    using table_type = std::unordered_map<std::string, std::uint32_t>;

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
    // Readers access immutable snapshots. Writers serialize updates and publish a new snapshot atomically.
    inline static std::shared_ptr<table_type const> table_ = std::make_shared<table_type>();
    inline static std::mutex writer_mutex_;
  };

inline bool Host_addr_tbl::insert(std::string const &host_name, std::uint32_t ipv4_addr)
  {
    std::lock_guard<std::mutex> lock(writer_mutex_);
    auto current = std::atomic_load_explicit(&table_, std::memory_order_acquire);
    if (current->find(host_name) != current->end())
      return false;

    auto updated = std::make_shared<table_type>(*current);
    updated->emplace(host_name, ipv4_addr);
    std::atomic_store_explicit(
      &table_, std::shared_ptr<table_type const>(std::move(updated)), std::memory_order_release);
    return true;
  }

inline bool Host_addr_tbl::remove(std::string const &host_name)
  {
    std::lock_guard<std::mutex> lock(writer_mutex_);
    auto current = std::atomic_load_explicit(&table_, std::memory_order_acquire);
    if (current->find(host_name) == current->end())
      return false;

    auto updated = std::make_shared<table_type>(*current);
    updated->erase(host_name);
    std::atomic_store_explicit(
      &table_, std::shared_ptr<table_type const>(std::move(updated)), std::memory_order_release);
    return true;
  }

inline std::uint32_t Host_addr_tbl::lookup(std::string const &host_name)
  {
    auto current = std::atomic_load_explicit(&table_, std::memory_order_acquire);
    auto const iter = current->find(host_name);
    return iter == current->end() ? 0 : iter->second;
  }
