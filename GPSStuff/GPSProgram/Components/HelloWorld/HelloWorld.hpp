// ======================================================================
// \title  HelloWorld.hpp
// \author axwemaxwell
// \brief  hpp file for HelloWorld component implementation class
// ======================================================================

#ifndef GPSProgram_HelloWorld_HPP
#define GPSProgram_HelloWorld_HPP

#include "GPSProgram/Components/HelloWorld/HelloWorldComponentAc.hpp"

namespace GPSProgram {

class HelloWorld final : public HelloWorldComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct HelloWorld object
    HelloWorld(const char* const compName  //!< The component name
    );

    //! Destroy HelloWorld object
    ~HelloWorld();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command SAY_HELLO
    //!
    //! TODO
    void SAY_HELLO_cmdHandler(FwOpcodeType opCode,              //!< The opcode
                              U32 cmdSeq,                       //!< The command sequence number
                              const Fw::CmdStringArg& greeting  //!< Greeting to repeat in the hello event
                              ) override;
};

}  // namespace GPSProgram

#endif
