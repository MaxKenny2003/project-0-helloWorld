// ======================================================================
// \title  GPSComponent.cpp
// \author axwemaxwell
// \brief  cpp file for GPSComponent component implementation class
// ======================================================================

#include "Project0Gps/Components/GPSComponent/GPSComponent.hpp"
#include <Fw/Logger/Logger.hpp>
#include <cctype>
 
namespace Project0Gps {
 
// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------
 
GPSComponent::GPSComponent(const char* const compName)
    : GPSComponentComponentBase(compName),
      m_lineLen(0),
      m_overflow(false),
      m_hasLock(false),
      m_noLockCount(0) {
    memset(m_line, 0, sizeof(m_line));
    memset(m_utc, 0, sizeof(m_utc));
}
 
GPSComponent::~GPSComponent() {}
 
// ----------------------------------------------------------------------
// Port handler: runs in the UART driver's read thread
// ----------------------------------------------------------------------
 
void GPSComponent::UartRead_handler(FwIndexType portNum,
                                    Fw::Buffer& buffer,
                                    const Drv::ByteStreamStatus& status) {
    if (status == Drv::ByteStreamStatus::OP_OK) {
        const U8* data = buffer.getData();
        for (FwSizeType i = 0; i < buffer.getSize(); i++) {
            this->handleByte(static_cast<char>(data[i]));
        }
    } else {
        this->log_WARNING_HI_UartReadError(status);
    }
 
    // Always give the buffer back to the BufferManager
    this->deallocate_out(0, buffer);
}
 
// ----------------------------------------------------------------------
// Command handler: runs in this component's thread
// ----------------------------------------------------------------------
 
void GPSComponent::SEND_UTC_TIME_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    char utc[sizeof(m_utc)];
 
    this->m_utcLock.lock();
    memcpy(utc, m_utc, sizeof(utc));
    this->m_utcLock.unlock();
 
    if (utc[0] == '\0') {
        snprintf(utc, sizeof(utc), "unavailable");
    }
 
    Fw::LogStringArg arg(utc);
    this->log_ACTIVITY_HI_UtcTime(arg);
 
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}
 
// ----------------------------------------------------------------------
// Sentence assembly: bytes arrive in arbitrary chunks
// ----------------------------------------------------------------------
 
void GPSComponent::handleByte(char c) {
    if (c == '$') {  // start of a new sentence, resync
        if (m_lineLen > 0) {
            Fw::Logger::log("[GPS] Parse error: incomplete sentence discarded\n");
        }
        m_lineLen = 0;
        m_overflow = false;
        m_line[m_lineLen++] = '$';
        return;
    }
    if (m_lineLen == 0) {  // not inside a sentence yet
        return;
    }
    if (c == '\r' || c == '\n') {  // end of sentence
        if (m_overflow) {
            Fw::Logger::log("[GPS] Parse error: sentence too long, discarded\n");
        } else {
            m_line[m_lineLen] = '\0';
            this->parseSentence(m_line);
        }
        m_lineLen = 0;
        m_overflow = false;
        return;
    }
    if (m_lineLen < MAX_SENTENCE - 1) {
        m_line[m_lineLen++] = c;
    } else {
        m_overflow = true;  // discard the rest of this line
    }
}
 
// ----------------------------------------------------------------------
// Sentence validation and dispatch
// ----------------------------------------------------------------------
 
bool GPSComponent::checksumOk(const char* line) {
    // checksum = XOR of everything between '$' and '*', as two hex digits
    const char* star = strchr(line, '*');
    if (line[0] != '$' || star == nullptr || strlen(star) != 3) {
        return false;
    }
    U8 sum = 0;
    for (const char* p = line + 1; p < star; p++) {
        sum ^= static_cast<U8>(*p);
    }
    char* end = nullptr;
    const unsigned long expected = strtoul(star + 1, &end, 16);
    if (end != star + 3) {
        return false;
    }
    return sum == static_cast<U8>(expected);
}
 
bool GPSComponent::allDigits(const char* s, FwSizeType count) {
    for (FwSizeType i = 0; i < count; i++) {
        if (!isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    return true;
}
 
F64 GPSComponent::nmeaToDegrees(const char* field) {
    // NMEA gives (d)ddmm.mmmm
    const F64 raw = atof(field);
    const F64 deg = static_cast<F64>(static_cast<int>(raw / 100.0));
    return deg + (raw - deg * 100.0) / 60.0;
}
 
void GPSComponent::parseSentence(char* line) {
    if (!checksumOk(line)) {
        Fw::Logger::log("[GPS] Parse error: bad checksum or malformed sentence: %.60s\n", line);
        return;
    }
 
    // Keep an untouched copy for error messages, since splitting edits in place
    char raw[MAX_SENTENCE];
    strncpy(raw, line, sizeof(raw) - 1);
    raw[sizeof(raw) - 1] = '\0';
 
    // Cut off "*hh" and split on commas in place, keeping empty fields
    *strchr(line, '*') = '\0';
    char* fields[MAX_FIELDS] = {nullptr};
    FwSizeType n = 0;
    char* p = line;
    while (p != nullptr && n < MAX_FIELDS) {
        fields[n++] = p;
        p = strchr(p, ',');
        if (p != nullptr) {
            *p++ = '\0';
        }
    }
 
    const char* id = fields[0];  // e.g. "$GNGGA"
    if (id[1] == 'P') {
        return;  // proprietary sentence (e.g. $PUBX), not an error
    }
    if (strlen(id) != 6) {
        Fw::Logger::log("[GPS] Parse error: unrecognized sentence ID: %.60s\n", raw);
        return;
    }
 
    const char* type = id + 3;  // skip '$' and the 2-letter talker ID
    if (strcmp(type, "GGA") == 0) {
        this->handleGga(fields, n, raw);
    } else if (strcmp(type, "RMC") == 0) {
        this->handleRmc(fields, n, raw);
    }
    // Other valid sentence types (GSV, GSA, VTG, GLL, TXT, ...) are ignored
}
 
// ----------------------------------------------------------------------
// GGA: $xxGGA,time,lat,N/S,lon,E/W,quality,numSats,hdop,alt,M,...
// ----------------------------------------------------------------------
 
void GPSComponent::handleGga(char** fields, FwSizeType n, const char* raw) {
    if (n < 10) {
        Fw::Logger::log("[GPS] Parse error: GGA has too few fields: %.60s\n", raw);
        return;
    }
 
    const int quality = atoi(fields[6]);  // 0 = no fix
    const U8 numSats = static_cast<U8>(atoi(fields[7]));
 
    if (quality > 0) {
        if (fields[2][0] == '\0' || fields[3][0] == '\0' ||
            fields[4][0] == '\0' || fields[5][0] == '\0') {
            Fw::Logger::log("[GPS] Parse error: GGA has a fix but empty position: %.60s\n", raw);
            return;
        }
 
        if (!m_hasLock) {
            m_hasLock = true;
            this->log_ACTIVITY_HI_GpsLockAcquired();
        }
        m_noLockCount = 0;
 
        F64 lat = nmeaToDegrees(fields[2]);
        if (fields[3][0] == 'S') {
            lat = -lat;
        }
        F64 lon = nmeaToDegrees(fields[4]);
        if (fields[5][0] == 'W') {
            lon = -lon;
        }
 
        this->tlmWrite_Latitude(lat);
        this->tlmWrite_Longitude(lon);
        if (fields[9][0] != '\0') {
            this->tlmWrite_Altitude(static_cast<F32>(atof(fields[9])));
        }
    } else {
        m_hasLock = false;
        // Event on the first unlocked sentence, then once per period
        if (m_noLockCount % NO_LOCK_EVENT_PERIOD == 0) {
            this->log_WARNING_HI_NoGpsLock(numSats);
        }
        m_noLockCount++;
    }
}
 
// ----------------------------------------------------------------------
// RMC: $xxRMC,time,status,lat,N/S,lon,E/W,speed,course,date,...
// ----------------------------------------------------------------------
 
void GPSComponent::handleRmc(char** fields, FwSizeType n, const char* raw) {
    if (n < 10) {
        Fw::Logger::log("[GPS] Parse error: RMC has too few fields: %.60s\n", raw);
        return;
    }
 
    const char* t = fields[1];  // hhmmss.ss
    const char* d = fields[9];  // ddmmyy
 
    // Before the receiver knows the time, these are empty: not an error
    if (t[0] == '\0' || d[0] == '\0') {
        return;
    }
    if (strlen(t) < 6 || strlen(d) != 6 || !allDigits(t, 6) || !allDigits(d, 6)) {
        Fw::Logger::log("[GPS] Parse error: RMC has a bad time or date: %.60s\n", raw);
        return;
    }
 
    char utc[sizeof(m_utc)];
    snprintf(utc, sizeof(utc), "20%c%c-%c%c-%c%c %c%c:%c%c:%c%c",
             d[4], d[5], d[2], d[3], d[0], d[1],
             t[0], t[1], t[2], t[3], t[4], t[5]);
 
    this->m_utcLock.lock();
    memcpy(m_utc, utc, sizeof(m_utc));
    this->m_utcLock.unlock();
 
    this->tlmWrite_UTCTime(Fw::TlmString(utc));
}
 
}  // namespace Project0Gps