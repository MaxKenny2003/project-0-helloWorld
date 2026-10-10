// ======================================================================
// \title  GPSComponent.hpp
// \author axwemaxwell
// \brief  hpp file for GPSComponent component implementation class
// ======================================================================

#ifndef Project0Gps_GPSComponent_HPP
#define Project0Gps_GPSComponent_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <Os/Mutex.hpp>
#include "Project0Gps/Components/GPSComponent/GPSComponentComponentAc.hpp"
 
namespace Project0Gps {
 
class GPSComponent final : public GPSComponentComponentBase {
  public:
    //! Construct GPSComponent object
    GPSComponent(const char* const compName);
 
    //! Destroy GPSComponent object
    ~GPSComponent();
 
  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------
 
    //! Handler implementation for UartRead
    void UartRead_handler(FwIndexType portNum,
                          Fw::Buffer& buffer,
                          const Drv::ByteStreamStatus& status) override;
 
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------
 
    //! Handler implementation for command SEND_UTC_TIME
    void SEND_UTC_TIME_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
 
    // ----------------------------------------------------------------------
    // Parsing helpers
    // ----------------------------------------------------------------------
 
    //! Feed one received character into the sentence assembler
    void handleByte(char c);
 
    //! Validate and dispatch one complete sentence (no CR/LF)
    void parseSentence(char* line);
 
    //! Handle a split GGA sentence (position, altitude, lock state)
    void handleGga(char** fields, FwSizeType n, const char* raw);
 
    //! Handle a split RMC sentence (UTC date and time)
    void handleRmc(char** fields, FwSizeType n, const char* raw);
 
    static bool checksumOk(const char* line);
    static bool allDigits(const char* s, FwSizeType count);
    static F64 nmeaToDegrees(const char* field);
 
    static constexpr FwSizeType MAX_SENTENCE = 128;  // NMEA max is 82 chars
    static constexpr FwSizeType MAX_FIELDS = 24;
 
    //! While unlocked, repeat the NoGpsLock event every this many GGA sentences
    static constexpr U32 NO_LOCK_EVENT_PERIOD = 10;
 
    // ----------------------------------------------------------------------
    // State
    // ----------------------------------------------------------------------
 
    char m_line[MAX_SENTENCE];  //!< sentence being assembled
    FwSizeType m_lineLen;
    bool m_overflow;
    char m_utc[40];             //!< latest "YYYY-MM-DD hh:mm:ss", empty if none
    bool m_hasLock;
    U32 m_noLockCount;
 
    Os::Mutex m_utcLock;        //!< guards m_utc (driver thread vs command thread)
};
 
}  // namespace Project0Gps
 
#endif
