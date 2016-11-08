This test program explores whether, in C++, using virtual functions rather than generic programming
(templates) can sometimes result in faster execution due to more cache hits.

The test involves, for instances of std::set instantiations, inserting a fixed number of elements
(given by Num_set_elems in common.h), and then erasing them in a different order.  (See
fill_empty_set() in main.cpp.)  For the template-only case, std::set is instantiated for pointers
to N different struct types.  (N is given by Num_S in common.h.)  All the struct types have a
(non-virtual) member function f(), the return value of which is used to compare pointers, and
order them in the set.  The fill/empty function is called for a set of each of the N types.  The
combined execution time for these N function calls is considered to be the generic programming
execution time.  Getting the polymorphic execution time involves a std::set instantiation for
pointers to an abstract struct.  This struct has a virtual member function, the return value of
which is used to compare pointers, and order them in the set.  For N distinct instances of this
std::set instantiation, the fill/emtpy function is called N times.

The expectation is that, as N increases, processor cache will get crowded with the N instantiations
of std::set<>::insert() and std::set<>::erase().  Which should lead to slower execution due to
cache evictions.  So, for large enough N, the polymorphic execution time should be less than the
generic programming execution time.
