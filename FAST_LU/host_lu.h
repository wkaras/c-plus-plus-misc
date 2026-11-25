#include <cstdint>
#include <string>
// Remove this comment, and add additional header files as necessary.  Not necessarily Standard Library header
// files.

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
    // Remove this comment, add private members as necessary.  Make all data member inline static.
  };

inline bool Host_addr_tbl::insert(std::string const &host_name, std::uint32_t ipv4_addr)
  {
    // Remove this comment, add function implementation.
  }

inline bool Host_addr_tbl::remove(std::string const &host_name)
  {
    // Remove this comment, add function implementation.
  }

inline std::uint32_t Host_addr_tbl::lookup(std::string const &host_name)
  {
    // Remove this comment, add function implementation.
  }
