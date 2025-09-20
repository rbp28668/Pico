

#include <stdint.h>

// Class to hold values and status for a single telemetry slot / channel.
class Slot {
    public:

    const static unsigned int UNIT_V = 1;  // one decimal
    const static unsigned int UNIT_A = 2;  // one decimal
    const static unsigned int UNIT_MS = 3;
    const static unsigned int UNIT_KPH = 4;
    const static unsigned int UNIT_RPM = 5;
    const static unsigned int UNIT_DEG_C = 6;  // one decimal place
    const static unsigned int UNIT_DEG_F = 7;
    const static unsigned int UNIT_METERS = 8;  // integer
    const static unsigned int UNIT_PCT_FUEL = 9;
    const static unsigned int UNIT_PCT_LQI = 10;
    const static unsigned int UNIT_MAH = 11;
    const static unsigned int UNIT_ML = 12;
    const static unsigned int UNIT_KM = 13; // KM, one decimal place
    const static unsigned int UNIT_NONE = 14;  // integer values.
    const static unsigned int UNIT_NONE2 = 15;  // integer values.
    private:
    // Use MS bit to determine whether slot is enabled.
    const static uint32_t ENABLED = 1<<31;

    // Nibble 0: Unit type
    // Nibble 1: Address
    // Nibble 2-5 : LS bit is alarm, rest is 15 bits signed value.
    // Top 2 nibbles for internal flags.
    uint32_t value;

    public:

    Slot() : value(0) {}

    void setId(int id) {
        value = (value & ~0xF0) | ((id & 0x0F) << 4);
    }

    void setUnits(int units){
        value = (value & ~0x0F) | (units & 0x0F);
    }
    void setAlarm(bool enabled) {
        value = (value & ~(1<<8)) | ((enabled ? 1 : 0) << 8);
    }

    void setValue(int val) {
        value = (value & ~(0x7FFF<<9)) | ((val & 0x7FFF) << 9);
    }

    void setEnabled(bool enabled) {
        value = (value & ~ENABLED) | (enabled ? ENABLED : 0);
    }

    uint32_t getValue() const { return value;}
    uint32_t getId() const { return (value & 0xF0)>>4;}
    uint8_t byte1() const {return (uint8_t) (value & 0xFF);}
    uint8_t byte2() const {return (uint8_t) ((value & 0xFF00) >> 8);}
    uint8_t byte3() const {return (uint8_t) ((value & 0xFF0000) >> 16);}

    bool isEnabled() const { return value & ENABLED;}

};


class Slots {
    public:
    const static int SLOT_COUNT = 16;
    
    private: 
    Slot slots[SLOT_COUNT];

    public:
    Slots(){
        for(int i=0; i<SLOT_COUNT; ++i){
            slots[i].setId(i);
        }
    }

    Slot* getSlot(int i){
        if(i<0) i = 0;
        if(i>= SLOT_COUNT) i=SLOT_COUNT-1;
        return slots+i;
    }
};

void start_telemetry();
Slots& telemetry_slots(); 
