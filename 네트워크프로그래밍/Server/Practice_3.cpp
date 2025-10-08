#include"Common.h"
#include <iostream>

void PrintEndianType() {
    WORD test = 0x1234; // 2바이트 변수

    unsigned char firstByte = LOBYTE(test); // 하위 1바이트 추출

    std::cout << "test 값: 0x" << std::hex << test << std::endl;
    std::cout << "test의 firstByte: 0x" << std::hex << (int)firstByte << std::endl;

    std::cout << "호스트의 바이트 정렬 방식: ";
    if (firstByte == 0x34) {
        std::cout << "리틀 엔디언 (Little Endian)" << std::endl;
    }
    else if (firstByte == 0x12) {
        std::cout << "빅 엔디언 (Big Endian)" << std::endl;
    }
    else {
        std::cout << "알 수 없음" << std::endl;
    }
}

int main(int argc, char* argv[])
{
    PrintEndianType();
	return 0;

}