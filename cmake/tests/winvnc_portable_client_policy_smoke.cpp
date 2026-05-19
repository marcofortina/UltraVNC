#include "vncPortableClientPolicy.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::portable;

int main()
{
    std::string reason;
    ClientConnectionPolicy shared(2);
    assert(shared.RegisterClient(true, &reason));
    assert(shared.RegisterClient(true, &reason));
    assert(!shared.CanAccept(true, &reason));
    assert(reason.find("maximum") != std::string::npos);
    shared.UnregisterClient(true);
    assert(shared.CanAccept(true, &reason));

    ClientConnectionPolicy exclusive;
    assert(exclusive.RegisterClient(false, &reason));
    assert(exclusive.HasExclusiveClient());
    assert(!exclusive.CanAccept(true, &reason));
    assert(reason.find("exclusive") != std::string::npos);
    exclusive.UnregisterClient(false);
    assert(!exclusive.HasExclusiveClient());
    assert(exclusive.RegisterClient(true, &reason));
    assert(!exclusive.CanAccept(false, &reason));
    assert(reason.find("exclusive") != std::string::npos);
    return 0;
}
