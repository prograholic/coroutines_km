#include "common.h"

bool ShouldConnectToDriver(int argc, char* argv [])
{
    if (argc != 2)
    {
        return false;
    }

    if (argv[1] == std::string("--connect_to_driver"))
    {
        return true;
    }

    return false;
}


int main(int argc, char* argv [])
{
    if (ShouldConnectToDriver(argc, argv))
    {
        ConnectToDriver();
    }
    else
    {
        TryWinApiWaiter();
        TRACE() << std::endl << std::endl;

        TryCoroWaiter();
        TRACE() << std::endl << std::endl;

        TestChronoAwait();
        TRACE() << std::endl << std::endl;
    }

    return 0;
}

