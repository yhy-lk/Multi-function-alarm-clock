

char max(char x, char y) {
    return x > y ? x : y;
}

char min(char x, char y) {
    return x > y ? y : x;
}


// 将targetNum修改changeNum，并将修改之后的值映射到区间[minNum, maxNum]
void targetNumChange(char* targetNum, char changeNum, char minNum, char maxNum) {
    *targetNum = *targetNum + changeNum;

    if (*targetNum > maxNum) {
        *targetNum = minNum + (*targetNum - maxNum - 1);
    }
    if (*targetNum < minNum) {
        *targetNum = maxNum - (minNum - *targetNum - 1);
    }
}
