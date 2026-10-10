module Project0Gps {

    @ Parses NMEA sentences from a u-blox NEO-M9N and reports GPS data
    active component GPSComponent {

        # ------------------------------------------------------------------
        # Commands
        # ------------------------------------------------------------------

        @ Emits a UtcTime event with the most recent GPS UTC time
        async command SEND_UTC_TIME opcode 0

        # ------------------------------------------------------------------
        # Events
        # ------------------------------------------------------------------

        @ Response to SEND_UTC_TIME
        event UtcTime(utc: string size 40) \
            severity activity high \
            format "Current GPS UTC time: {}"

        @ GPS reports no valid position fix (repeats periodically while unlocked)
        event NoGpsLock(numSats: U8) \
            severity warning high \
            format "GPS has no lock ({} satellites used)"

        @ GPS went from no lock to a valid fix
        event GpsLockAcquired \
            severity activity high \
            format "GPS lock acquired"

        @ The UART driver reported a bad status
        event UartReadError(status: Drv.ByteStreamStatus) \
            severity warning high \
            format "UART read failed: {}"

        # ------------------------------------------------------------------
        # Telemetry
        # ------------------------------------------------------------------

        @ UTC date and time, "YYYY-MM-DD hh:mm:ss"
        telemetry UTCTime: string size 40

        @ Latitude in degrees (negative = south)
        telemetry Latitude: F64 format "{.6f} deg"

        @ Longitude in degrees (negative = west)
        telemetry Longitude: F64 format "{.6f} deg"

        @ Altitude above mean sea level
        telemetry Altitude: F32 format "{.1f} m"

        # ------------------------------------------------------------------
        # Data ports
        # ------------------------------------------------------------------

        @ Filled buffers from the UART driver's $recv port
        sync input port UartRead: Drv.ByteStreamData

        @ Returns used buffers to the BufferManager
        output port deallocate: Fw.BufferSend

        # ------------------------------------------------------------------
        # Standard AC ports
        # ------------------------------------------------------------------

        @ Port for requesting the current time
        time get port timeCaller

        @ Enables command handling
        import Fw.Command

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channel handling
        import Fw.Channel
    }
}
