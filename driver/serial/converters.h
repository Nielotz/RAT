#ifndef CONVERTERS_H
#define CONVERTERS_H
#include <vector>

template<typename SourceType, typename TargetType>
void convert32bitTo4(SourceType number, std::vector<TargetType>& target, const int arrayOffset = 0) {
    target[0 + arrayOffset] = static_cast<TargetType>((number & 0xff000000) >> 24);
    target[1 + arrayOffset] = static_cast<TargetType>((number & 0xff0000) >> 16);
    target[2 + arrayOffset] = static_cast<TargetType>((number & 0xff00) >> 8);
    target[3 + arrayOffset] = static_cast<TargetType>(number & 0xff);
}

template<typename SourceType, typename TargetType>
std::vector<TargetType> convert32bitTo4(SourceType number) {
    std::vector<TargetType> converted(4);
    convert32bitTo4<SourceType, TargetType>(number, converted);
    return converted;
}

template<typename SourceType, typename TargetType>
TargetType convert4x8BitsTo32(const std::vector<SourceType>& array, const int arrayOffset = 0) {
    return array[0 + arrayOffset] << 24
           | array[1 + arrayOffset] << 16
           | array[2 + arrayOffset] << 8
           | array[3 + arrayOffset];
}

#endif //CONVERTERS_H
