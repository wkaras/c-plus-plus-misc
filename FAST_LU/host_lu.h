#include <cstdint>
#include <string>
#include <mutex>
#include <atomic>
#include <ru_shared_mutex.h>
#include <list.h>
#include <hash_table.h>

#pragma once

/*
Lookup table of IPv4 address from host-name.  All public member functions are thread safe.  The hash table is
typically expected to have fewer than 4096 entries.
*/
class Host_addr_tbl
  {
  public:

    // Insert new host name.  If host name already in table, returns false and does nothing.
    //
    static bool insert(std::string const &host_name, std::uint32_t ipv4_addr);

    // Delete host name.  If host name not in table, returns false and does nothing.
    //
    static bool remove(std::string const &host_name);

    // Lookup host name and return corresponding IPv4 address.  Returns zero if host name not in table.  Calling
    // this will never block the thread.  Return value may not reflect any in-progress insert or delete in another
    // thread.  The implementation is optimized to minimize the latency of this function call.
    //
    static std::uint32_t lookup(std::string const &host_name);

    // This class is for scoping only, no instances.
    //
    Host_addr_tbl() = delete;

  private:

    // Chain-bucket hash table node that can be in hash table 0 and hash table 1.
    //
    struct node_type
      {
        std::string host_name;
        std::uint32_t ipv4_addr;

        // link[0] is for hash table 0, link[1] is for hash table 1.
        node_type *link[2];
      };

    template <unsigned hash_table_num>
    struct list_abstractor
      {
        static const bool store_tail{false};

        using handle = node_type *;

        static handle null() { return nullptr; }

        static void link(handle h, handle link_h) { h->link[hash_table_num] = link_h; }

        static handle link(handle h) { return h->link[hash_table_num]; }
      };

    template <unsigned hash_table_num>
    struct hash_tbl_abstractor
      {
        using list = abstract_container::list<list_abstractor<hash_table_num> >;

        using index = std::size_t;

        using key = const std::string *;

        static const index num_hash_values{2 * 4096};

        static index hash_key(key k) { return std::hash<std::string>()(*k) % num_hash_values; }

        static index hash_elem(typename list::handle h) { return hash_key(&(h->host_name)); }

        static bool is_key(key k, typename list::handle h) { return *k == h->host_name; }
      };

    static inline abstract_container::hash_table<hash_tbl_abstractor<0> > hash_tbl_0;

    static inline abstract_container::hash_table<hash_tbl_abstractor<1> > hash_tbl_1;

    static inline abstract_container::ru_shared_mutex::id id0, id1;

    // Shared mutex to protect multi-thread access to hash_table 0.
    //
    using sh_mtx_0_type = abstract_container::ru_shared_mutex::c<id0>;
    sh_mtx_0_type & sh_mtx_0 = sh_mtx_0_type::inst();

    // Shared mutex to protect multi-thread access to hash_table 1.
    //
    using sh_mtx_1_type = abstract_container::ru_shared_mutex::c<id1>;
    sh_mtx_1_type & sh_mtx_1 = sh_mtx_1_type::inst();
  };

inline bool Host_addr_tbl::insert(std::string const &host_name, std::uint32_t ipv4_addr)
  {
    sh_mtx_0.lock();
    sh_mtx_1.lock();

    if (hash_tbl_0.search(&host_name) != hash_tbl_0.null())
      {
        sh_mtx_1.unlock();
        sh_mtx_0.unlock();
        return false;
      }

    node_type *node = new node_type{host_name, ipv4_addr, {nullptr, nullptr}};

    hash_tbl_0.insert(node);
    hash_tbl_1.insert(node);

    sh_mtx_1.unlock();
    sh_mtx_0.unlock();
    return true;
  }

inline bool Host_addr_tbl::remove(std::string const &host_name)
  {
    sh_mtx_0.lock();
    sh_mtx_1.lock();

    node_type *node = hash_tbl_0.remove_key(&host_name);
    if (node == hash_tbl_0.null())
      {
        sh_mtx_1.unlock();
        sh_mtx_0.unlock();
        return false;
      }

    hash_tbl_1.remove(node);
    delete node;

    sh_mtx_1.unlock();
    sh_mtx_0.unlock();
    return true;
  }

inline std::uint32_t Host_addr_tbl::lookup(std::string const &host_name)
  {
    sh_mtx_0.lock_shared();
    node_type *node = hash_tbl_0.search(&host_name);

    if (node == hash_tbl_0.null())
      {
        sh_mtx_0.unlock_shared();
        sh_mtx_1.lock_shared();
        node = hash_tbl_1.search(&host_name);
        sh_mtx_1.unlock_shared();

        return node == hash_tbl_1.null() ? 0 : node->ipv4_addr;
      }

    std::uint32_t addr = node->ipv4_addr;
    sh_mtx_0.unlock_shared();
    return addr;
  }
