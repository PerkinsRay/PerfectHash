#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <numeric>
#include <unordered_set>
#include <vector>
#define HASHDEBUG(...)
// #define HASHDEBUG(...) printf(__VA_ARGS__)

#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x)   __builtin_expect(!!(x), 1)
static size_t get_prime(size_t Min) {
    static size_t            MaxPrime = 7;
    static std::vector<bool> prime{ 0, 0, 1, 1, 0, 1, 0, 1 };
    while (MaxPrime < Min) {
        const size_t origSize = prime.size();
        prime.resize(origSize << 1, 1);
        for (size_t idx = 2; idx < origSize; idx++) {
            if (prime[idx] == 0) continue;
            size_t start = origSize / idx * idx;
            start        = (start == idx) ? (start + idx) : start;
            for (size_t pos = start; pos < (origSize << 1); pos += idx) {
                prime[pos] = 0;
            }
        }
        for (size_t idx = (origSize << 1) - 1; idx > MaxPrime; idx--) {
            if (prime[idx]) {
                MaxPrime = idx;
                break;
            }
        }
    }
    for (size_t idx = Min; idx < prime.size(); idx++) {
        if (prime[idx]) {
            return idx;
        }
    }
    return MaxPrime;
}
static size_t get_mask(size_t s) {
    size_t mask = -1ULL;
    if (s == 0) return 0;
    while ((s & (mask >> 1)) == s) {
        mask >>= 1;
    }
    return mask;
}

constexpr size_t shift(size_t val, size_t s) { return val >> s; }

template <size_t SZ, size_t V = 0>
struct lg2 {
    static constexpr int val = lg2<shift(SZ, 1), V + 1>::val;
};

template <size_t V>
struct lg2<0, V> {
    static constexpr int val = V;
};

template <size_t V>
struct lg2<1, V> {
    static constexpr int val = V;
};

template <size_t SZ>
struct StringHasher {
    static constexpr uint64_t GOLD  = 0x9e3779b97f4a7c15ULL;
    static constexpr int      SHIFT = 64 - lg2<SZ>::val;

    static size_t hash(const char *str, size_t pm) {
        uint64_t res = str[0] * GOLD;
        for (uint32_t i = 1; str[i] != 0; i++) {
            res ^= str[i] * GOLD;
        }
        return (res * pm) >> SHIFT;
    }
};

template <class CONTENT, size_t SZ, class HASHER>
class PerfectHash {
    struct HASH_PARAM {
        uint16_t                 hash_pm1{ 0 };
        uint16_t                 hash_pm2{ 0 };
        uint16_t                 hash_mask{ 0 };
        size_t                   hash_max{ 0 };
        std::array<uint16_t, SZ> hash_mapping1{};
        std::array<uint16_t, SZ> hash_mapping2{};
        std::array<CONTENT, SZ>  hash_keys{};
    };
    struct Condition {
        CONTENT I;
        size_t  v1;
        size_t  v2;
    };
    struct hash_value {
        uint16_t                 v{ 0xffff };
        std::vector<Condition *> asso;
        hash_value() { asso.reserve(8); }
    };

    HASH_PARAM hash_param;

    inline bool check_cond(Condition *cond, std::vector<bool> &final_val, std::array<hash_value, SZ> &mapping1,
                           std::array<hash_value, SZ> &mapping2, std::vector<bool> &addition_val, size_t mask) const {
        if (mapping1[cond->v1].v != 0xffff && mapping2[cond->v2].v != 0xffff) {
            auto res = (mapping1[cond->v1].v + mapping2[cond->v2].v) & mask;
            if (res >= final_val.size() || final_val[res] || addition_val[res]) {
                return false;
            }
            addition_val[res] = true;
        }
        return true;
    }
    bool check_asso(std::vector<bool> &final_val, std::array<hash_value, SZ> &mapping1,
                    std::array<hash_value, SZ> &mapping2, size_t mask, const hash_value &hv) const {
        std::vector<bool> addition_val(final_val.size(), false);
        for (auto *cond : hv.asso) {
            if (!check_cond(cond, final_val, mapping1, mapping2, addition_val, mask)) {
                return false;
            }
        }
        return true;
    }
    bool apply_mapping_value(std::vector<bool> &final_val, std::array<hash_value, SZ> &mapping1,
                             std::array<hash_value, SZ> &mapping2, size_t mask, const hash_value &hv) const {
        for (auto *cond : hv.asso) {
            if (mapping1[cond->v1].v != 0xffff && mapping2[cond->v2].v != 0xffff) {
                auto res       = (mapping1[cond->v1].v + mapping2[cond->v2].v) & mask;
                final_val[res] = true;
            }
        }
        return true;
    }
    bool revert_mapping_value(std::vector<bool> &final_val, std::array<hash_value, SZ> &mapping1,
                              std::array<hash_value, SZ> &mapping2, size_t mask, const hash_value &hv) const {
        for (auto *cond : hv.asso) {
            if (mapping1[cond->v1].v != 0xffff && mapping2[cond->v2].v != 0xffff) {
                auto res       = (mapping1[cond->v1].v + mapping2[cond->v2].v) & mask;
                final_val[res] = false;
            }
        }
        return true;
    }
    bool check_next_asso(std::vector<bool> &final_val, std::array<hash_value, SZ> &mapping1,
                         std::array<hash_value, SZ> &mapping2, size_t mask, const hash_value &hv, int32_t depth) {

        for (auto *cond : hv.asso) {
            bool a = find_mapping1(final_val, mapping1, mapping2, cond->v1, depth + 1);
            if (!a) return false;
            bool b = find_mapping2(final_val, mapping1, mapping2, cond->v2, depth + 1);
            if (!b) return false;
        }
        return true;
    }
    bool find_mapping1(std::vector<bool> &final_val, std::array<hash_value, SZ> &mapping1,
                       std::array<hash_value, SZ> &mapping2, uint16_t id, int32_t depth) {
        hash_value &hv = mapping1[id];
        if (hv.v != 0xffff) return true;
        size_t mask = hash_param.hash_mask;
        for (uint16_t i = 0; i < SZ; i++) {
            hv.v = i;
            if (check_asso(final_val, mapping1, mapping2, mask, hv)) {
                apply_mapping_value(final_val, mapping1, mapping2, mask, hv);
                if (check_next_asso(final_val, mapping1, mapping2, mask, hv, depth)) {
                    return true;
                }
                revert_mapping_value(final_val, mapping1, mapping2, mask, hv);
                i += ((i >> 1) & 0xfffe);
                if (depth > 3) {
                    break;
                }
            }
        }
        hv.v = 0xffff;
        return false;
    }
    bool find_mapping2(std::vector<bool> &final_val, std::array<hash_value, SZ> &mapping1,
                       std::array<hash_value, SZ> &mapping2, uint16_t id, int32_t depth) {
        hash_value &hv = mapping2[id];
        if (hv.v != 0xffff) return true;
        size_t mask = hash_param.hash_mask;
        for (uint16_t i = 0; i < SZ; i++) {
            hv.v = i;
            if (check_asso(final_val, mapping1, mapping2, mask, hv)) {
                apply_mapping_value(final_val, mapping1, mapping2, mask, hv);
                if (check_next_asso(final_val, mapping1, mapping2, mask, hv, depth)) {
                    return true;
                }
                revert_mapping_value(final_val, mapping1, mapping2, mask, hv);
                i += ((i >> 1) & 0xfffe);
                if (depth > 2) {
                    break;
                }
            }
        }
        hv.v = 0xffff;
        return false;
    }

    bool find_mapping_with_order(std::vector<bool> &final_val, std::array<hash_value, SZ> &mapping1,
                                 const std::array<uint16_t, SZ> &order_for_mapping1,
                                 std::array<hash_value, SZ>     &mapping2,
                                 const std::array<uint16_t, SZ> &order_for_mapping2) {
        size_t i = 0, j = 0;
        for (; i < SZ && j < SZ;) {
            uint16_t id1 = order_for_mapping1[i];
            uint16_t id2 = order_for_mapping2[j];
            if (mapping1[id1].asso.size() > mapping2[id2].asso.size()) {
                bool res = find_mapping1(final_val, mapping1, mapping2, id1, 0);
                if (unlikely(!res)) return false;
                i++;
            } else {
                bool res = find_mapping2(final_val, mapping1, mapping2, id2, 0);
                if (unlikely(!res)) return false;
                j++;
            }
        }
        for (; i < SZ; i++) {
            uint16_t id1 = order_for_mapping1[i];
            bool     res = find_mapping1(final_val, mapping1, mapping2, id1, 0);
            if (unlikely(!res)) return false;
        }
        for (; j < SZ; j++) {
            uint16_t id2 = order_for_mapping2[j];
            bool     res = find_mapping2(final_val, mapping1, mapping2, id2, 0);
            if (unlikely(!res)) return false;
        }
        return true;
    }

    template <class CONTENTS>
    std::vector<Condition> initial_condition(size_t prime1, size_t prime2, const CONTENTS &keys) const {
        std::vector<Condition> conds;
        conds.reserve(keys.size());
        for (const CONTENT &I : keys) {
            size_t hash1 = HASHER::hash(I, prime1);
            size_t hash2 = HASHER::hash(I, prime2);
            HASHDEBUG("Hash %s:%zu,%zu\n", I.Instrument, hash1, hash2);
            conds.push_back({ I, hash1, hash2 });
        }
        return conds;
    }

    bool check_hash_uniq(const std::vector<Condition> &conds) const {
        std::unordered_set<uint64_t> uniq;
        for (auto &cond : conds) {
            uint64_t v = (cond.v1 << 32) | cond.v2;
            if (uniq.find(v) == uniq.end()) {
                uniq.insert(v);
            } else {
                HASHDEBUG("Not uniq key!\n");
                return false;
            }
        }
        return true;
    }

    std::array<uint16_t, SZ> build_dfs_order(std::array<hash_value, SZ> &mapping) const {
        std::array<uint16_t, SZ> order;
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(),
                  [&](uint16_t a, uint16_t b) { return mapping[a].asso.size() > mapping[b].asso.size(); });
        return order;
    }

    template <class CONTENTS>
    bool find_mapping(size_t r1, size_t r2, std::array<hash_value, SZ> &mapping1, std::array<hash_value, SZ> &mapping2,
                      CONTENTS &keys) {
        std::vector<Condition> conds = initial_condition(r1, r2, keys);
        if (!check_hash_uniq(conds)) return false;

        for (auto &cond : conds) {
            mapping1[cond.v1].asso.push_back(&cond);
            mapping2[cond.v2].asso.push_back(&cond);
        }

        std::vector<bool> final_val(keys.size(), false);

        auto order_for_mapping1 = build_dfs_order(mapping1);
        auto order_for_mapping2 = build_dfs_order(mapping2);

        return find_mapping_with_order(final_val, mapping1, order_for_mapping1, mapping2, order_for_mapping2);
    }

    template <class CONTENTS>
    void validate(const CONTENTS &keys) {
        for (const CONTENT &I : keys) {
            size_t hash_val = hash(I);
            if (hash_val < maxHash() && !hash_param.hash_keys[hash_val]) {
                hash_param.hash_keys[hash_val] = I;
            } else {
                printf("validate fail!\n");
                return;
            }
        }
        for (size_t i = maxHash(); i <= get_mask(maxHash()); i++) {
            hash_param.hash_keys[i] = hash_param.hash_keys[0];
        }
        for (size_t i = 0; i <= get_mask(maxHash()); i++) {
            HASHDEBUG("Hash:%s->%zu(%zu)\n", hash_param.hash_keys[i].Instrument, hash(hash_param.hash_keys[i]), i);
        }
        printf("validate succeed\n");
    }

  public:
    inline size_t maxHash() const { return hash_param.hash_max; };

    void clear() { *hash_param = HASH_PARAM{}; }
    PerfectHash() {}
    ~PerfectHash() {}
    size_t hash(const CONTENT &I) const {
        return (hash_param.hash_mapping1[HASHER::hash(I, hash_param.hash_pm1)] +
                hash_param.hash_mapping2[HASHER::hash(I, hash_param.hash_pm2)]) &
               hash_param.hash_mask;
    }
    const CONTENT &check_hash(size_t Idx) const { return hash_param.hash_keys[Idx]; }

    void commit() {}

    void setup(const std::initializer_list<CONTENT> &keys) { setup<std::initializer_list<CONTENT>>(keys); }

    template <class CONTENTS>
    void setup(const CONTENTS &keys) {
        hash_param  = HASH_PARAM{};
        size_t mask = get_mask(keys.size());
        if (mask > SZ) {
            fprintf(stderr, "too many keys,%zu,limit:%zu", keys.size(), SZ);
            return;
        }
        hash_param.hash_mask = mask;
        printf("PerfectHash: Key size:%zu,mask:%zu,finding primes", keys.size(), mask);
        fflush(stdout);
        while (true) {
            size_t a = get_prime(rand() & (mask << 2));
            size_t b = get_prime(rand() & (mask << 2));
            if (a == b) b = get_prime(b + 1);
            std::array<hash_value, SZ> mapping1;
            std::array<hash_value, SZ> mapping2;
            if (find_mapping(a, b, mapping1, mapping2, keys)) {
                printf("\nSuccess Hash_Pm %zu %zu\n", a, b);
                for (size_t i = 0; i < SZ; i++) {
                    hash_param.hash_mapping1[i] = mapping1[i].v;
                    hash_param.hash_mapping2[i] = mapping2[i].v;
                }
                hash_param.hash_pm1 = a;
                hash_param.hash_pm2 = b;
                break;
            } else {
                printf(".");
                fflush(stdout);
            }
        }
        hash_param.hash_max = keys.size();
        validate(keys);
    }
};
