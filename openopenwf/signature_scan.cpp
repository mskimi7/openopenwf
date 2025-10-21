#include "openwf.h"

std::vector<unsigned char*> SignatureScan(const char* patternChar, const char* mask, unsigned char* data, size_t length)
{
    const unsigned char* pattern = (const unsigned char*)patternChar;

    size_t maskSize = strlen(mask);
    const char* findWild = strrchr(mask, '?');

    ptrdiff_t last[256];
    std::fill(std::begin(last), std::end(last), findWild ? (findWild - mask) : -1);

    for (size_t i = 0; i < maskSize; ++i)
    {
        if (last[pattern[i]] < (ptrdiff_t)i)
            last[pattern[i]] = i;
    }

    std::vector<unsigned char*> results;

    for (unsigned char* i = data, *end = data + length - maskSize; i <= end;)
    {
        ptrdiff_t j = maskSize - 1;

        while ((j >= 0) && (mask[j] == '?' || pattern[j] == i[j]))
            j--;

        if (j < 0)
            results.emplace_back(i++);
        else
            i += std::max((ptrdiff_t)1, j - last[i[j]]);
    }

    return results;
}
