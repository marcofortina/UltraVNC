#include "vncPortableFileTransfer.h"

#include <cassert>
#include <string>

using namespace uvnc::winvnc::portable;

int main()
{
    FileTransferMode mode = FileTransferMode::Disabled;
    assert(ParseFileTransferMode("disabled", mode));
    assert(mode == FileTransferMode::Disabled);
    assert(ParseFileTransferMode("reject-only", mode));
    assert(mode == FileTransferMode::RejectOnly);
    assert(!ParseFileTransferMode("enabled", mode));

    FileTransferMessage message;
    message.contentType = rfbFileHeader;
    message.contentParam = 7;
    message.size = 42;
    message.length = 128;

    FileTransferDecision decision = EvaluateFileTransferMessage(message, FileTransferMode::Disabled, 1024);
    assert(!decision.accepted);
    assert(decision.readPayload);
    assert(decision.payloadBytes == 128);
    assert(decision.abortReason == 7);
    assert(decision.reason == "file transfer is disabled");

    decision = EvaluateFileTransferMessage(message, FileTransferMode::RejectOnly, 1024);
    assert(!decision.accepted);
    assert(decision.readPayload);
    assert(decision.payloadBytes == 128);
    assert(decision.reason.find("reject-only") != std::string::npos);

    message.length = 4096;
    decision = EvaluateFileTransferMessage(message, FileTransferMode::RejectOnly, 1024);
    assert(!decision.accepted);
    assert(!decision.readPayload);
    assert(decision.payloadBytes == 0);
    assert(decision.reason.find("guard limit") != std::string::npos);

    return 0;
}
