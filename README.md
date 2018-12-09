# Tiny85RotaryEncoderI2C
This program make ATTiny85 to I2C Rotary-Encoder decoder device.
build this source and write to Tiny85, it worsk as I2C device.
default I2C Address is 0x38.
this device will decode from 0 to 255 with loop(default). 

[How to use]  
1. Build this source and write to Tiny85. // Board:Attiny Core  
2. Connect Tiny85 and Rotary encoder (EncA,B,Common and Button below).  
3. Connect Tiny85 and Your Arduino (SCL-SCL,SDA-SDA,Vcc-Vcc and EncCmn-Gnd).  
4. Start connection with Wire.begin();  

[PIN]  
          ........  
       RST|1    8|Vcc  
      EncA|2    7|SCL  
      EncB|3    6|EncBtn  
    EncCmn|4    5|SDA  
          ........  

[Command]  
    COMMAND       ID           // bytes:Description  
    ENC_CMD_INIT (0x01)        // 1:Reset  
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
    if(cntV==220){ // reset test
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

    
![I2C AT Tiny85](https://camo.qiitausercontent.com/bc7f5e0a156dea4b734dea37badd3774b7421d5d/68747470733a2f2f71696974612d696d6167652d73746f72652e73332e616d617a6f6e6177732e636f6d2f302f35393931312f65396633623033302d656661612d653039332d346436352d6536356265666466396637332e6a706567)
