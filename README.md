# Tiny85RotaryEncoderI2C
This program make ATTiny85 to I2C Rotary-Encoder decoder.
build this source and burn to Tiny85, it worsk as I2C device.
default I2C Address is 0x38.
this device decode from 0 to 255 with loop(default). 

[How to use]
1. build this source and write to Tiny85. // Board:Attiny Core
2. connect Tiny85 and Rotary encoder (EncA,B,Cmn and Btn below).
3. connect Tiny85 and Your Arduino (SCL-SCL,SDA-SDA,Vcc-Vcc and EncCmn-Gnd).
4. start connection with   Wire.begin();
[PIN] 

        --------
     RST|1    8|Vcc
    EncA|2    7|SCL
    EncB|3    6|EncBtn
    EncCmn|4    5|SDA
        --------
        
[Command]

    COMMAND       ID           // bytes:Description
    ENC_CMD_INIT (0x01)        // 1:Init/Reset
    ENC_REQ_GET_VAL_U8 (0x10)  // 1:GetRotValue
    ENC_REQ_GET_BTN (0x12)     // 1:GetBtn/Trg/Rel and flush
    ENC_CMD_LOOP (0x03)        // 1:Loop ON/OFF
    ENC_CMD_ACCEL (0x02)       // 1:CountUpSpeedThreshold

[sample source to use]

    // I2C Master for Tiny85RotaryEncoder
    #include <Wire.h>
    #include "Tiny85RotaryEncDef.h"

    void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    Wire.begin();
    delay(100);
    sendCmd(ENC_CMD_LOOP,0);
    Serial.println("Loop Off");
    }

    uint32_t oldCntV=0;
    uint32_t oldCntB=0;
    void loop() {
    // put your main code here, to run repeatedly:
    uint32_t cntV = readReq(ENC_REQ_GET_VAL_INT32,0);
    uint32_t cntB = readReq(ENC_REQ_GET_BTN,0);
    uint32_t cntS = readReq(ENC_REQ_GET_SPD,0);
    if((oldCntV!=cntV)||(oldCntB!=cntB)){
        oldCntV=cntV;
        oldCntB=cntB;
        Serial.println("recv:"+String(cntV)+":"+String(cntB)+":"+String(cntS));
    }
    if(cntV==220){ // reset
        sendCmd(ENC_CMD_INIT,0);
    }
    delay(1);
    }

    int32_t sendCmd(uint8_t cmdId, uint8_t param) {
    Wire.beginTransmission(ENC_I2C_ADDR);
    Wire.write(cmdId); // cmd
    Wire.write(param); // data
    Wire.endTransmission();
    }
    int32_t readReq(uint8_t reqId, uint8_t reqData) {
    uni32 u; u.num = 0;
    Wire.beginTransmission(ENC_I2C_ADDR);
    Wire.write(reqId); // cmd
    Wire.write(reqData); // data
    Wire.endTransmission();
    int dataCnt = getParamNum(reqId);
    Wire.requestFrom(ENC_I2C_ADDR,dataCnt); // req dataCnt byte
    for(int i=0; i < dataCnt; ++i){
        if(Wire.available()){
        u.parts[i] = Wire.read();
        }
    }
    return u.num;
    }
