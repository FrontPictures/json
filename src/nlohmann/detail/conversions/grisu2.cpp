#include "grisu2.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

grisu2::cached_power grisu2::get_cached_power_for_binary_exponent(int e)
{
    constexpr int kCachedPowersMinDecExp = -300;
    constexpr int kCachedPowersDecStep = 8;

    static constexpr std::array<cached_power, 79> kCachedPowers = {{
        {0xAB70FE17C79AC6CA, -1060, -300}, {0xFF77B1FCBEBCDC4F, -1034, -292},
        {0xBE5691EF416BD60C, -1007, -284}, {0x8DD01FAD907FFC3C, -980, -276},
        {0xD3515C2831559A83, -954, -268},  {0x9D71AC8FADA6C9B5, -927, -260},
        {0xEA9C227723EE8BCB, -901, -252},  {0xAECC49914078536D, -874, -244},
        {0x823C12795DB6CE57, -847, -236},  {0xC21094364DFB5637, -821, -228},
        {0x9096EA6F3848984F, -794, -220},  {0xD77485CB25823AC7, -768, -212},
        {0xA086CFCD97BF97F4, -741, -204},  {0xEF340A98172AACE5, -715, -196},
        {0xB23867FB2A35B28E, -688, -188},  {0x84C8D4DFD2C63F3B, -661, -180},
        {0xC5DD44271AD3CDBA, -635, -172},  {0x936B9FCEBB25C996, -608, -164},
        {0xDBAC6C247D62A584, -582, -156},  {0xA3AB66580D5FDAF6, -555, -148},
        {0xF3E2F893DEC3F126, -529, -140},  {0xB5B5ADA8AAFF80B8, -502, -132},
        {0x87625F056C7C4A8B, -475, -124},  {0xC9BCFF6034C13053, -449, -116},
        {0x964E858C91BA2655, -422, -108},  {0xDFF9772470297EBD, -396, -100},
        {0xA6DFBD9FB8E5B88F, -369, -92},   {0xF8A95FCF88747D94, -343, -84},
        {0xB94470938FA89BCF, -316, -76},   {0x8A08F0F8BF0F156B, -289, -68},
        {0xCDB02555653131B6, -263, -60},   {0x993FE2C6D07B7FAC, -236, -52},
        {0xE45C10C42A2B3B06, -210, -44},   {0xAA242499697392D3, -183, -36},
        {0xFD87B5F28300CA0E, -157, -28},   {0xBCE5086492111AEB, -130, -20},
        {0x8CBCCC096F5088CC, -103, -12},   {0xD1B71758E219652C, -77, -4},
        {0x9C40000000000000, -50, 4},      {0xE8D4A51000000000, -24, 12},
        {0xAD78EBC5AC620000, 3, 20},       {0x813F3978F8940984, 30, 28},
        {0xC097CE7BC90715B3, 56, 36},      {0x8F7E32CE7BEA5C70, 83, 44},
        {0xD5D238A4ABE98068, 109, 52},     {0x9F4F2726179A2245, 136, 60},
        {0xED63A231D4C4FB27, 162, 68},     {0xB0DE65388CC8ADA8, 189, 76},
        {0x83C7088E1AAB65DB, 216, 84},     {0xC45D1DF942711D9A, 242, 92},
        {0x924D692CA61BE758, 269, 100},    {0xDA01EE641A708DEA, 295, 108},
        {0xA26DA3999AEF774A, 322, 116},    {0xF209787BB47D6B85, 348, 124},
        {0xB454E4A179DD1877, 375, 132},    {0x865B86925B9BC5C2, 402, 140},
        {0xC83553C5C8965D3D, 428, 148},    {0x952AB45CFA97A0B3, 455, 156},
        {0xDE469FBD99A05FE3, 481, 164},    {0xA59BC234DB398C25, 508, 172},
        {0xF6C69A72A3989F5C, 534, 180},    {0xB7DCBF5354E9BECE, 561, 188},
        {0x88FCF317F22241E2, 588, 196},    {0xCC20CE9BD35C78A5, 614, 204},
        {0x98165AF37B2153DF, 641, 212},    {0xE2A0B5DC971F303A, 667, 220},
        {0xA8D9D1535CE3B396, 694, 228},    {0xFB9B7CD9A4A7443C, 720, 236},
        {0xBB764C4CA7A44410, 747, 244},    {0x8BAB8EEFB6409C1A, 774, 252},
        {0xD01FEF10A657842C, 800, 260},    {0x9B10A4E5E9913129, 827, 268},
        {0xE7109BFBA19C0C9D, 853, 276},    {0xAC2820D9623BF429, 880, 284},
        {0x80444B5E7AA7CF85, 907, 292},    {0xBF21E44003ACDD2D, 933, 300},
        {0x8E679C2F5E44FF8F, 960, 308},    {0xD433179D9C8CB841, 986, 316},
        {0x9E19DB92B4E31BA9, 1013, 324},
    }};
    const int f = kAlpha - e - 1;
    const int k = (f * 78913) / (1 << 18) + static_cast<int>(f > 0);

    const int index = (-kCachedPowersMinDecExp + k + (kCachedPowersDecStep - 1))
                      / kCachedPowersDecStep;

    const cached_power cached = kCachedPowers[static_cast<size_t>(index)];

    return cached;
}

int grisu2::find_largest_pow10(const uint32_t n, uint32_t &pow10)
{
    if (n >= 1000000000) {
        pow10 = 1000000000;
        return 10;
    }

    if (n >= 100000000) {
        pow10 = 100000000;
        return 9;
    }
    if (n >= 10000000) {
        pow10 = 10000000;
        return 8;
    }
    if (n >= 1000000) {
        pow10 = 1000000;
        return 7;
    }
    if (n >= 100000) {
        pow10 = 100000;
        return 6;
    }
    if (n >= 10000) {
        pow10 = 10000;
        return 5;
    }
    if (n >= 1000) {
        pow10 = 1000;
        return 4;
    }
    if (n >= 100) {
        pow10 = 100;
        return 3;
    }
    if (n >= 10) {
        pow10 = 10;
        return 2;
    }

    pow10 = 1;
    return 1;
}

void grisu2::grisu2_round(char *buf, int len, uint64_t dist, uint64_t delta, uint64_t rest,
                          uint64_t ten_k)
{
    while (rest < dist && delta - rest >= ten_k
           && (rest + ten_k < dist || dist - rest > rest + ten_k - dist)) {
        buf[len - 1]--;
        rest += ten_k;
    }
}

void grisu2::grisu2_digit_gen(char *buffer, int &length, int &decimal_exponent, diyfp M_minus,
                              diyfp w, diyfp M_plus)
{
    uint64_t delta = diyfp::sub(M_plus, M_minus).f;
    uint64_t dist = diyfp::sub(M_plus, w).f;

    const diyfp one(uint64_t{1} << -M_plus.e, M_plus.e);

    auto p1 = static_cast<std::uint32_t>(M_plus.f >> -one.e);
    uint64_t p2 = M_plus.f & (one.f - 1);

    std::uint32_t pow10{};
    const int k = find_largest_pow10(p1, pow10);

    int n = k;
    while (n > 0) {
        const std::uint32_t d = p1 / pow10;
        const std::uint32_t r = p1 % pow10;

        buffer[length++] = static_cast<char>('0' + d);

        p1 = r;
        n--;

        const uint64_t rest = (uint64_t{p1} << -one.e) + p2;
        if (rest <= delta) {
            decimal_exponent += n;

            const uint64_t ten_n = uint64_t{pow10} << -one.e;
            grisu2_round(buffer, length, dist, delta, rest, ten_n);

            return;
        }

        pow10 /= 10;
    }

    int m = 0;
    for (;;) {
        p2 *= 10;
        const uint64_t d = p2 >> -one.e;
        const uint64_t r = p2 & (one.f - 1);

        buffer[length++] = static_cast<char>('0' + d);

        p2 = r;
        m++;

        delta *= 10;
        dist *= 10;
        if (p2 <= delta) {
            break;
        }
    }

    decimal_exponent -= m;

    const uint64_t ten_m = one.f;
    grisu2_round(buffer, length, dist, delta, p2, ten_m);
}

void grisu2::grisu2(char *buf, int &len, int &decimal_exponent, diyfp m_minus, diyfp v,
                    diyfp m_plus)
{
    const cached_power cached = get_cached_power_for_binary_exponent(m_plus.e);

    const diyfp c_minus_k(cached.f, cached.e);

    const diyfp w = diyfp::mul(v, c_minus_k);
    const diyfp w_minus = diyfp::mul(m_minus, c_minus_k);
    const diyfp w_plus = diyfp::mul(m_plus, c_minus_k);

    const diyfp M_minus(w_minus.f + 1, w_minus.e);
    const diyfp M_plus(w_plus.f - 1, w_plus.e);

    decimal_exponent = -cached.k;

    grisu2_digit_gen(buf, len, decimal_exponent, M_minus, w, M_plus);
}

void grisu2::grisu2(char *buf, int &len, int &decimal_exponent, double value)
{
    static_assert(diyfp::kPrecision >= std::numeric_limits<double>::digits + 3,
                  "internal error: not enough precision");

#if 0
    const boundaries w = compute_boundaries(static_cast<double>(value));
#else
    const boundaries w = compute_boundaries(value);
#endif

    grisu2(buf, len, decimal_exponent, w.minus, w.w, w.plus);
}

char *grisu2::append_exponent(char *buf, int e)
{
    if (e < 0) {
        e = -e;
        *buf++ = '-';
    } else {
        *buf++ = '+';
    }

    auto k = static_cast<std::uint32_t>(e);
    if (k < 10) {
        *buf++ = '0';
        *buf++ = static_cast<char>('0' + k);
    } else if (k < 100) {
        *buf++ = static_cast<char>('0' + k / 10);
        k %= 10;
        *buf++ = static_cast<char>('0' + k);
    } else {
        *buf++ = static_cast<char>('0' + k / 100);
        k %= 100;
        *buf++ = static_cast<char>('0' + k / 10);
        k %= 10;
        *buf++ = static_cast<char>('0' + k);
    }

    return buf;
}

char *grisu2::format_buffer(char *buf, int len, int decimal_exponent, int min_exp, int max_exp)
{
    const int k = len;
    const int n = len + decimal_exponent;

    if (k <= n && n <= max_exp) {
        std::memset(buf + k, '0', static_cast<size_t>(n) - static_cast<size_t>(k));

        buf[n + 0] = '.';
        buf[n + 1] = '0';
        return buf + (static_cast<size_t>(n) + 2);
    }

    if (0 < n && n <= max_exp) {
        std::memmove(buf + (static_cast<size_t>(n) + 1), buf + n,
                     static_cast<size_t>(k) - static_cast<size_t>(n));
        buf[n] = '.';
        return buf + (static_cast<size_t>(k) + 1U);
    }

    if (min_exp < n && n <= 0) {
        std::memmove(buf + (2 + static_cast<size_t>(-n)), buf, static_cast<size_t>(k));
        buf[0] = '0';
        buf[1] = '.';
        std::memset(buf + 2, '0', static_cast<size_t>(-n));
        return buf + (2U + static_cast<size_t>(-n) + static_cast<size_t>(k));
    }

    if (k == 1) {
        buf += 1;
    } else {
        std::memmove(buf + 2, buf + 1, static_cast<size_t>(k) - 1);
        buf[1] = '.';
        buf += 1 + static_cast<size_t>(k);
    }

    *buf++ = 'e';
    return append_exponent(buf, n - 1);
}

grisu2::boundaries grisu2::compute_boundaries(double value)
{
    constexpr int kPrecision = std::numeric_limits<double>::digits;
    constexpr int kBias = std::numeric_limits<double>::max_exponent - 1 + (kPrecision - 1);
    constexpr int kMinExp = 1 - kBias;
    constexpr uint64_t kHiddenBit = uint64_t{1} << (kPrecision - 1);

    using bits_type = typename std::conditional<kPrecision == 24, std::uint32_t, uint64_t>::type;

    const auto bits = static_cast<uint64_t>(reinterpret_bits<bits_type>(value));
    const uint64_t E = bits >> (kPrecision - 1);
    const uint64_t F = bits & (kHiddenBit - 1);

    const bool is_denormal = E == 0;
    const diyfp v = is_denormal ? diyfp(F, kMinExp)
                                : diyfp(F + kHiddenBit, static_cast<int>(E) - kBias);

    const bool lower_boundary_is_closer = F == 0 && E > 1;
    const diyfp m_plus = diyfp(2 * v.f + 1, v.e - 1);
    const diyfp m_minus = lower_boundary_is_closer ? diyfp(4 * v.f - 1, v.e - 2)
                                                   : diyfp(2 * v.f - 1, v.e - 1);

    const diyfp w_plus = diyfp::normalize(m_plus);

    const diyfp w_minus = diyfp::normalize_to(m_minus, w_plus.e);

    return {diyfp::normalize(v), w_minus, w_plus};
}
