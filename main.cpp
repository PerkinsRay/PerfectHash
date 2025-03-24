#include "perfect_hash.hpp"
#include <initializer_list>
#include <iostream>

int32_t main() {
    PerfectHash<const char *, 1024, StringHasher<1024>> ph;
    std::initializer_list<const char *> values = { "I have never told this story about the little prince",
                                                   "Now I would like to tell this story for you",
                                                   "I still remember a little prince",
                                                   "I’m little prince",
                                                   "I come from a small planet",
                                                   "I have a rose and I love her so much" };
    ph.setup(values);
    for (auto value : values) {
        std::cout << "value " << value << " :\nhash:" << ph.hash(value) << std::endl;
    }
    return 0;
}
