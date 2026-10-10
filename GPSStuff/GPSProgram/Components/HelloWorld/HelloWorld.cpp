// ======================================================================
// \title  HelloWorld.cpp
// \author axwemaxwell
// \brief  cpp file for HelloWorld component implementation class
// ======================================================================

#include "GPSProgram/Components/HelloWorld/HelloWorld.hpp"

namespace GPSProgram {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

HelloWorld ::HelloWorld(const char* const compName) : HelloWorldComponentBase(compName) {}

HelloWorld ::~HelloWorld() {}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void HelloWorld ::SAY_HELLO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg& greeting) {
    // TODO
    Fw::LogStringArg eventGreeting(greeting.toChar());
    this->log_ACTIVITY_HI_Hello(eventGreeting);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);

}

}  // namespace GPSProgram
