#ifndef _TINY85ROTARYENCDEF_H_
#define _TINY85ROTARYENCDEF_H_

#define ENC_I2C_ADDR (0x38) // 7bit
#define ENC_CMD_PARAM_SIZE (4)

#define ENC_BUTTON (1)
#define ENC_ENC0 (3)
#define ENC_ENC1 (4)

#define ENC_CMD_INIT (0x01) // 1:Init
#define ENC_CMD_ACCEL (0x02) // 1:ChangeSPeedUpRate
#define ENC_CMD_LOOP (0x03) // 1:SetLoop
#define ENC_CMD_INVERT (0x04) // 1:SetInvert
#define ENC_REQ_GET_VAL_U8 (0x10) // 1:GetU8
#define ENC_REQ_GET_VAL_INT32 (0x11) // 4:GetInt32
#define ENC_REQ_GET_BTN (0x12) // 1:GetBtn/Trg/Rel and flush
#define ENC_REQ_GET_BTN_NO_FLUSH (0x13) // 1:GetBtn/Trg/Rel and no flush
#define ENC_REQ_GET_SPD (0x14) // 4:GetU32

union uni32{
    int32_t num;
    uint8_t parts[4];
};

int32_t Wire_read32(){
  uni32 u;
  for(int i=0;i<4;++i){
    u.parts[i]=Wire.read();
  }
  return u.num;
}
void Wire_write32(int32_t v){
  uni32 u;
  u.num = v;
  for(int i=0;i<4;++i){
    Wire.write(u.parts[i]);
  }
}

uint8_t getParamNum(uint8_t _cmd){
  uint8_t ret = 1; // default
  switch(_cmd){
    case ENC_REQ_GET_VAL_INT32:
    case ENC_REQ_GET_SPD:
      ret = 4;
      break;
  }
  return ret;
}

#endif // _TINY85ROTARYENCDEF_H_
