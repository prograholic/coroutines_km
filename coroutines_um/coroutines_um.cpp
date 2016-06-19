#include "common.h"


int main()
{
    TryWinApiWaiter();
    TRACE() << std::endl << std::endl;

    TryCoroWaiter();
    TRACE() << std::endl << std::endl;

    TestChronoAwait();
    TRACE() << std::endl << std::endl;

    return 0;
}

