#include <gmock/gmock.h>

int _argc;
char** _argv;

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    testing::InitGoogleMock(&argc, argv);
    _argc = argc;
    _argv = argv;
    return RUN_ALL_TESTS();
}
