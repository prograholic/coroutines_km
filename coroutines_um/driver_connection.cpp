#include "common.h"

#include <std_emu.h>
#include <driver_connection.h>

void ConnectToDriver()
{
    std_emu::HandleGuard driverConnection(::CreateFile(CORO_PUBLIC_NAME_STRING,
                                                       0,
                                                       0,
                                                       nullptr,
                                                       OPEN_EXISTING,
                                                       0,
                                                       nullptr));

    if (!driverConnection.valid())
    {
        std_emu::error_code ec = std_emu::GetLastErrorCode();
        std::cerr << "failed to open driver: " << ec.value() << std::endl;
        return;
    }

    std::cerr << "driver successfully opened" << std::endl;
}
