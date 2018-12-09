//---------------------------
// Rotary decoder for Tyny85
//---------------------------
// Board:Attiny Core
#include <EEPROM.h>

#define ENC_EPROM_ADDR_I2CADR (0)  // 1(reserved)
#define ENC_EPROM_ADDR_IsLoop (1)  // 1
#define ENC_EPROM_ADDR_IsInvert (2)  // 1
#define ENC_EPROM_ADDR_EncVal (4)  // 4
#define ENC_EPROM_ADDR_EncMin (8)  // 4
#define ENC_EPROM_ADDR_EncMax (12) // 4
template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    int i;
    for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
    return i;
}
template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    int i;
    for (i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
    return i;
}
  
class Tyny85RotEnc{
public:
private:
  volatile int mPinb = 0;
  volatile int mEncVal = 0;
  volatile byte mBtnState = 0;
  volatile bool mIsDirty = false;
  volatile int mDeltaSum = 0;
  volatile uint32_t mOldMillis=0;
  volatile uint32_t mEncRotDeltaMillis=0;
  byte mPBEnc0;
  byte mPBEnc1;
  byte mPBBtn;
  byte mPBMask;
  int32_t mEncOld;
  int32_t mEncMin = 0;
  int32_t mEncMax = 255;
  bool mIsLoop = false;
  bool mIsInvert = false;
  uint32_t mSpeedAcc = 5;
  uint32_t mDeltaMillis=0;
  uint32_t mSpeedupMillis=3;
  uint32_t mChatterMillis=1;

public:
  int GetValue(){ return mEncVal; }
  bool GetBtn(){ return (mBtnState&1)?true:false; }
  bool GetBtnTrg(){ return (mBtnState&2)?true:false; }
  bool GetBtnRel(){ return (mBtnState&4)?true:false; }
  bool FlushBtn(){ return mBtnState&=0B001; } // clear trg&rel
  uint32_t GetEncRotDeltaMillis(){ return mEncRotDeltaMillis; }
  void SetEncMin(int32_t _min=0){ mEncMin = _min; }
  void SetEncMax(int32_t _max=255){ mEncMax = _max; }
  void SetLoop(bool _isLoop=true){ mIsLoop = _isLoop; }
  void SetInvert(bool _isInvert=true){ mIsInvert = _isInvert; }
  
  Tyny85RotEnc(byte _enc0, byte _enc1, byte _btn){
    mPBEnc0 = _enc0;
    mPBEnc1 = _enc1;
    mPBBtn = _btn;
    mPBMask = (1<<mPBEnc0)|(1<<mPBEnc1)|(1<<mPBBtn);
  }

  void SetDefaultValue(int32_t _defVal, int32_t _valMin=0, int32_t _valMax=255, bool _isLoop=false){
    mEncOld = mEncVal = _defVal;
    mEncMin = _valMin;
    mEncMax = _valMax;
    mIsLoop = _isLoop;
    mIsInvert = false;
    mDeltaSum = 0;
    mIsDirty = true;
    EEPROM_writeAnything(ENC_EPROM_ADDR_EncVal,_defVal); //4
    EEPROM_writeAnything(ENC_EPROM_ADDR_EncMin,_valMin); //4
    EEPROM_writeAnything(ENC_EPROM_ADDR_EncMax,_valMax); //4
    EEPROM_writeAnything(ENC_EPROM_ADDR_IsLoop,_isLoop); //1
    EEPROM_writeAnything(ENC_EPROM_ADDR_IsInvert,mIsInvert); //1
  }
  void RestoreDefaultValue(){
    EEPROM_readAnything(ENC_EPROM_ADDR_EncVal,mEncVal); //4
    EEPROM_readAnything(ENC_EPROM_ADDR_EncMin,mEncMin); //4
    EEPROM_readAnything(ENC_EPROM_ADDR_EncMax,mEncMax); //4
    EEPROM_readAnything(ENC_EPROM_ADDR_IsLoop,mIsLoop); //1
    EEPROM_writeAnything(ENC_EPROM_ADDR_IsInvert,mIsInvert); //1
    mEncOld = mEncVal;
    mDeltaSum = 0;
    mIsDirty = true;
  }

  void setup() {
    pinMode(mPBBtn, INPUT_PULLUP);
    pinMode(mPBEnc0, INPUT_PULLUP);
    pinMode(mPBEnc1, INPUT_PULLUP);
    // Configure pin change interrupts on Enc0,1, and Btn
    PCMSK |= mPBMask;
    GIMSK = 1 << PCIE;     // Enable pin change interrupts
    GIFR = 1 << PCIF;      // Clear pin change interrupt flag.
    //  sei(); //turn interrupts on
  }

  void loop(uint32_t _deltaMillis) {
    mDeltaMillis += _deltaMillis;
    if(mIsDirty){
    noInterrupts();
      mIsDirty = false;
      mEncVal+=mDeltaSum;
      mDeltaSum = 0;
      mDeltaMillis = 0;
      if(mIsLoop){
        if(mEncVal<mEncMin){
          mEncVal+=(mEncMax+1);
        }else if(mEncVal>mEncMax){
          mEncVal-=(mEncMax+1);
        }
      }
      mEncVal = max(min(mEncVal,mEncMax),mEncMin);
      mEncOld = mEncVal;
    interrupts();
    }
  }

  void IsrImpl(){
//    noInterrupts();
    uint32_t nowMillis = millis();
    uint32_t intDeltaMillis = nowMillis - mOldMillis;
    volatile int newPinb = PINB;
    bool dirtyEnc = UpdateRotEnc(mPinb,newPinb,intDeltaMillis); // PB1,3 and 4
    if(dirtyEnc){
      mEncRotDeltaMillis = intDeltaMillis;
      mOldMillis = nowMillis;
    }
    mIsDirty |= dirtyEnc;
    mPinb = newPinb;
//    interrupts();
  }

  bool UpdateRotEnc(int _oldPINB, int _newPINB, unsigned long _deltaMillis){
    bool isDirtyEnc = false;    
    int portReading = _newPINB & mPBMask; // EncA,B and Btn
    int portReadingOld = _oldPINB & mPBMask; // EncA,B and Btn
    if(portReading != portReadingOld){
      int addVal = 0;
      int rotState = ((portReading & (1 << mPBEnc0))?1:0) | ((portReading & (1 << mPBEnc1))?2:0);
      int rotStateOld = ((portReadingOld & (1 << mPBEnc0))?1:0) | ((portReadingOld & (1 << mPBEnc1))?2:0);
      if((rotStateOld==0B11)&&(mChatterMillis < _deltaMillis)){
        if(rotState==0B01){
          addVal = -1*(mIsInvert?-1:1);
        } else if(rotState==0B10){
          addVal = 1*(mIsInvert?-1:1);
        }
        if(addVal!=0){
          if(mSpeedupMillis > _deltaMillis){
            addVal *= mSpeedAcc;
          }        
          mDeltaSum += addVal;
          isDirtyEnc = true;
        }
      }
      bool btn = (portReading & (1 << mPBBtn));
      bool btnOld = (portReadingOld & (1 << mPBBtn));
      if(btn!=btnOld){
        mBtnState = (mBtnState&0B110)|(!btn?1:0); // on
        mBtnState |= (!btn && btnOld)?(1<<1):0;  // trg:on
        mBtnState |= (btn && !btnOld)?(1<<2):0;  // rel:trg:on
      }
    }
    return isDirtyEnc;
  }

  void Reset(){
    RestoreDefaultValue();
  }

  void SetAccel(uint8_t _accel=5, uint8_t _timeSpan=2){
    mSpeedAcc = (uint32_t)_accel;
    mSpeedupMillis = (uint32_t)_timeSpan;
  }
};

//---------------------------
// I2C device
//     --------
//  RST|1    8|VCC
// EncA|2    7|SCL
// EncB|3    6|EncBtn
//  GND|4    5|SDA
//     --------
//---------------------------
  #include <Wire.h>
#include "Tiny85RotaryEncDef.h"

Tyny85RotEnc rotEnc = Tyny85RotEnc(ENC_ENC0,ENC_ENC1,ENC_BUTTON);
uint32_t oldMillis;
volatile uint8_t v_cmd;
volatile uint8_t v_params[ENC_CMD_PARAM_SIZE];
unsigned long m_delayMillis=0;

void setup() {
  // put your setup code here, to run once:
  oldMillis = millis();
  rotEnc.setup();
  rotEnc.SetDefaultValue(128,0,255,true);
//  rotEnc.restoreDefaultValue();

  Wire.begin(ENC_I2C_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void loop(){
  uint32_t nowMillis = millis();
  uint32_t deltaMillis = nowMillis - oldMillis;
  oldMillis = nowMillis;
  rotEnc.loop(deltaMillis);
  if(m_delayMillis>0){
    delay(m_delayMillis);
  }
}

ISR (PCINT0_vect) {
  rotEnc.IsrImpl();
}

void receiveEvent(int howMany) {
//  noInterrupts();
  if(Wire.available()){
    v_cmd = Wire.read();
    int i=0;
    while (Wire.available()||(i<ENC_CMD_PARAM_SIZE-1)){
      v_params[i] = Wire.read();
      i++;
    }
    while (Wire.available()){
      uint8_t dmy = Wire.read();
    }
  }
  switch(v_cmd){
    case ENC_CMD_INIT:   req_ENC_CMD_INIT();  break;
    case ENC_CMD_ACCEL:  req_ENC_CMD_ACCEL(v_params[0]);  break;
    case ENC_CMD_LOOP:   req_ENC_CMD_LOOP(v_params[0]);  break;
    case ENC_CMD_INVERT: req_ENC_CMD_INVERT(v_params[0]);  break;
  }
//  interrupts();
}

void requestEvent(){
//  noInterrupts();
  uint8_t data = v_cmd;
  switch(v_cmd){
    case ENC_REQ_GET_VAL_U8:
      req_ENC_REQ_GET_VAL_U8();
      break;
    case ENC_REQ_GET_VAL_INT32:
      req_ENC_REQ_GET_VAL_INT32();
      break;
    case ENC_REQ_GET_BTN:
    case ENC_REQ_GET_BTN_NO_FLUSH:
      req_ENC_REQ_GET_BTN(v_cmd==ENC_REQ_GET_BTN);
      break;
    case ENC_REQ_GET_SPD:
      req_ENC_ENC_REQ_GET_SPD();
      break;
  }
  if(v_cmd==ENC_REQ_GET_VAL_U8){
  }else if((v_cmd==ENC_REQ_GET_BTN)||(v_cmd==ENC_REQ_GET_BTN_NO_FLUSH)){
  }
 // interrupts();
}

void req_ENC_CMD_INIT(){
  rotEnc.Reset(); // Trg,Relを消去
}
void req_ENC_CMD_ACCEL(uint8_t spd){
  rotEnc.SetAccel(spd);
}
void req_ENC_CMD_LOOP(uint8_t _isLoop){
  rotEnc.SetLoop(_isLoop);
}
void req_ENC_CMD_INVERT(uint8_t _isInvert){
  rotEnc.SetInvert(_isInvert);
}
void req_ENC_REQ_GET_VAL_U8(){
    uint8_t data = (uint8_t)(rotEnc.GetValue()&0xff);
    Wire.write(data);
}
void req_ENC_REQ_GET_VAL_INT32(){
    uint32_t data = (uint32_t)(rotEnc.GetValue()&0xff);
    Wire_write32(data);
}
void req_ENC_REQ_GET_BTN(bool _flush){
    uint8_t data = rotEnc.GetBtn() ? 1:0;
    data |= rotEnc.GetBtnTrg() ? (1<<1):0;
    data |= rotEnc.GetBtnRel() ? (1<<2):0;
    if(v_cmd==ENC_REQ_GET_BTN){
      rotEnc.FlushBtn(); // Flush Trg and Rel
    }
    Wire.write(data);
}
void req_ENC_ENC_REQ_GET_SPD(){
    uint32_t sm = rotEnc.GetEncRotDeltaMillis();
    Wire_write32(sm);
}

