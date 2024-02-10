#ifndef CONVERTERS_H
#define CONVERTERS_H
#include <vector>

template<typename SourceType, typename TargetType>
void convert32bitTo4(SourceType number, TargetType *target) {
    target[0] = static_cast<TargetType>((number & 0xff000000) >> 24);
    target[1] = static_cast<TargetType>((number & 0xff0000) >> 16);
    target[2] = static_cast<TargetType>((number & 0xff00) >> 8);
    target[3] = static_cast<TargetType>(number & 0xff);
}

template<typename SourceType, typename TargetType>
std::vector<TargetType> convert32bitTo4(SourceType number) {
    std::vector<TargetType> converted(4);
    convert32bitTo4<SourceType, TargetType>(number, converted.data());
    return converted;
}


#endif //CONVERTERS_H
