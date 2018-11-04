#include "util/HSMIDI.h"

// Each packet is 32 bytes, because a single SysEx message needs to be pretty small for Teensy USB
// MIDI. The EEPROM is 2048 bytes of storage, and the first 128 bytes are used for calibration. So
// the total number of SysEx packets is (2048 - 128) / 32 = 60
#define NUMBER_OF_PACKETS 60

class XFER: public SystemExclusiveHandler {
public:
	void Init() {
		receiving = 0;
		packet = 0;
	}

    void Controller() {
    		if (receiving) ListenForSysEx();
    }
    
    void View() {DrawInterface();}
    
    void ToggleReceiveMode() {
    		receiving = 1 - receiving;
    		packet = 0;
    }
    
    void OnSendSysEx() {
    		if (!receiving) {
    			packet = 0;
            uint8_t V[33];
    			
    			uint16_t address = EEPROM_CALIBRATIONDATA_END;
    			for (byte p = 0; p < NUMBER_OF_PACKETS; p++)
    			{
    				packet = p;
    				uint8_t ix = 0;
    				V[ix++] = packet; // Packet number
    				// Wrap into 32-byte packets
    				for (byte b = 0; b < 32; b++) 
    				{
    					V[ix++] = EEPROM.read(address++);
    				}
    				
    				UnpackedData unpacked;
    				unpacked.set_data(ix, V);
    				PackedData packed = unpacked.pack();
    				SendSysEx(packed, '_'); // _ indicates O_C
    			}
    		}
    }
    
    void OnReceiveSysEx() {
    		uint8_t V[33];
    		if (ExtractSysExData(V, '_')) {
    			uint8_t ix = 0;
    			uint8_t p = V[ix++]; // Get packet number
    			packet = p;
    			uint16_t address = (p * 32) + EEPROM_CALIBRATIONDATA_END;
    			for (byte b = 0; b < 32; b++)
    			{
    				EEPROM.write(address++, V[ix++]);
    			}
    			if (p == NUMBER_OF_PACKETS - 1) {
    				receiving = 0;
    				OC::apps::Init(0);
    			}
    		}
    }
        
private:
    bool receiving;
    uint8_t packet;
    
    void DrawInterface() {
        menu::DefaultTitleBar::Draw();
        graphics.setPrintPos(0, 1);
        graphics.print("MIDI Transfer");
        
        graphics.setPrintPos(0, 15);
        if (receiving) {
        		if (packet > 0) {
        			graphics.print("Receiving...");

        			// Progress bar
        			graphics.drawRect(0, 33, (packet + 4) * 2, 8);
        		}
        		else graphics.print("Listening...");
        } else {
        		if (packet > 0) graphics.print("Done!");
        		else graphics.print("Receive or Send?");
        }
        
		graphics.setPrintPos(0, 55);
        if (receiving) graphics.print("[CANCEL]");
        else {
        		graphics.print("[RECEIVE]");
        		graphics.setPrintPos(90, 55);
        		graphics.print("[SEND]");
        }
    }
    
};

XFER XFER_instance;

void XFER_init() {}
void XFER_menu() {XFER_instance.View();}
void XFER_isr() {XFER_instance.Controller();}

// Storage not used for this app
size_t XFER_storageSize() {return 0;}
size_t XFER_save(void *storage) {return 0;}
size_t XFER_restore(const void *storage) {return 0;}

void XFER_handleAppEvent(OC::AppEvent event) {
    if (event == OC::APP_EVENT_RESUME) XFER_instance.Init();
}
void XFER_loop() {} // Deprecated
void XFER_screensaver() {XFER_instance.View();}
void XFER_handleEncoderEvent(const UI::Event &event) {}

void XFER_handleButtonEvent(const UI::Event &event) {
	if (event.type == UI::EVENT_BUTTON_PRESS) {
	    if (event.control == OC::CONTROL_BUTTON_L) XFER_instance.ToggleReceiveMode();
	    if (event.control == OC::CONTROL_BUTTON_R) XFER_instance.OnSendSysEx();
	}
}
