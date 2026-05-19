#include "vncPortableMemoryServer.h"
#include "vncPortableRfb.h"
#include "vncPortableRfbMessages.h"

#include <cassert>
#include <cstring>
#include <string>
#include <thread>

using namespace uvnc::winvnc::portable;

namespace {

class StaticClipboardSource : public RfbClipboardSource {
public:
    explicit StaticClipboardSource(const std::string& text) : text_(text) {}
    bool GetText(std::string& text, std::string *) const override
    {
        text = text_;
        return true;
    }
private:
    std::string text_;
};

bool RunHandshake(TcpSocket& client)
{
    char version[sz_rfbProtocolVersionMsg] = {};
    if (!client.ReadExact(version, sizeof(version))) return false;
    const std::string clientVersion = ProtocolVersion38();
    if (!client.WriteAll(clientVersion.data(), clientVersion.size())) return false;
    CARD8 security[2] = {};
    if (!client.ReadExact(security, sizeof(security))) return false;
    if (security[0] != 1 || security[1] != rfbNoAuth) return false;
    const CARD8 selected = rfbNoAuth;
    if (!client.WriteAll(&selected, sizeof(selected))) return false;
    CARD32 securityResult = 1;
    if (!client.ReadExact(&securityResult, sizeof(securityResult)) || securityResult != AuthOkValue()) return false;
    rfbClientInitMsg init = {};
    init.flags = clientInitShared;
    if (!client.WriteAll(&init, sz_rfbClientInitMsg)) return false;
    rfbServerInitMsg serverInit = {};
    if (!client.ReadExact(&serverInit, sz_rfbServerInitMsg)) return false;
    const CARD32 nameLength = Swap32IfLE(serverInit.nameLength);
    std::string name(nameLength, '\0');
    return name.empty() || client.ReadExact(&name[0], name.size());
}

} // namespace

int main()
{
    ServerConfig config;
    config.SetAllowNoAuth(true);
    config.SetPort(0);
    config.SetSize(16, 16);

    MemoryServer server;
    assert(server.Start(config));

    StaticClipboardSource source("clipboard-from-backend");
    bool accepted = false;
    bool serverOk = false;
    std::thread worker([&]() {
        serverOk = server.TryServeOneUpdates(1, nullptr, 0, accepted, nullptr, &source);
    });

    TcpSocket client;
    bool clientOk = TcpSocket::Connect("127.0.0.1", server.Port(), client) && RunHandshake(client);
    rfbServerCutTextMsg cut = {};
    clientOk = clientOk && client.ReadExact(&cut, sz_rfbServerCutTextMsg);
    clientOk = clientOk && cut.type == rfbServerCutText;
    const CARD32 length = Swap32IfLE(cut.length);
    std::string text(length, '\0');
    clientOk = clientOk && (text.empty() || client.ReadExact(&text[0], text.size()));
    clientOk = clientOk && text == "clipboard-from-backend";

    FramebufferUpdateRequest request = {};
    request.incremental = false;
    request.width = 16;
    request.height = 16;
    const rfbFramebufferUpdateRequestMsg wire = EncodeFramebufferUpdateRequest(request);
    clientOk = clientOk && client.WriteAll(&wire, sz_rfbFramebufferUpdateRequestMsg);
    rfbFramebufferUpdateMsg update = {};
    clientOk = clientOk && client.ReadExact(&update, sz_rfbFramebufferUpdateMsg);

    worker.join();
    server.Stop();
    assert(accepted);
    assert(serverOk);
    assert(clientOk);
    return 0;
}
