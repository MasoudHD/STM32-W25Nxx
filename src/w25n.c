#include "w25n.h"


static SPI_HandleTypeDef _hspi;

static int _dieSelect = 0;
static uint32_t _cs = 0;
static GPIO_TypeDef *_GPIOx;
static int _model = -1;


void W25N_ERROR(void)
{
    while(1)
    {
        __NOP();
    }
}


int w25nInit(GPIO_TypeDef *GPIOx, uint32_t cs, SPI_HandleTypeDef *hspi)
{
    _hspi = *hspi;
    _cs = cs;
    _GPIOx = GPIOx;
    digitalWrite(_GPIOx, _cs, HIGH);

    w25nReset();

    char jedec[5] = {0x9F, 0x00, 0x00, 0x00, 0x00};
    w25nSendData(jedec, sizeof(jedec));
    if(jedec[2] == (char)WINBOND_MAN_ID)
    {
        if((uint16_t)(jedec[3] << 8 | jedec[4]) == W25N01GV_DEV_ID)
        {
            w25nSetStatusReg(W25N_PROT_REG, 0x00);
            _model = W25N01GV;
            return 0;
        }
        if((uint16_t)(jedec[3] << 8 | jedec[4]) == W25M02GV_DEV_ID)
        {
            _model = W25M02GV;
            W25nDieSelect(0);
            w25nSetStatusReg(W25N_PROT_REG, 0x00);
            W25nDieSelect(1);
            w25nSetStatusReg(W25N_PROT_REG, 0x00);
            W25nDieSelect(0);
            return 0;
        }
    }
    W25N_ERROR(); //return 1;
    return 1;
}

enum chipModels w25nGetModel(void)
{
  return _model;
}

void w25nSendData(char * buf, uint32_t len)
{
    digitalWrite(_GPIOx, _cs, LOW);
    HAL_SPI_TransmitReceive(&_hspi, (uint8_t*)buf, (uint8_t*)buf, len, 100);
    digitalWrite(_GPIOx, _cs, HIGH);
}

void w25nReset()
{
    char buf[] = {W25N_RESET};
    w25nSendData(buf, sizeof(buf));
    HAL_Delay(1);
}

void w25nSetStatusReg(char reg, char set)
{
  char buf[3] = {W25N_WRITE_STATUS_REG, reg, set};
  w25nSendData(buf, sizeof(buf));
}

char w25nGetStatusReg(char reg)
{
  char buf[3] = {W25N_READ_STATUS_REG, reg, 0x00};
  w25nSendData(buf, sizeof(buf));
  return buf[2];
}

uint32_t w25nGetMaxPage()
{
  if (_model == W25M02GV) return W25M02GV_MAX_PAGE;
  if (_model == W25N01GV) return W25N01GV_MAX_PAGE;
  return 0;
}

int w25nDieSelectOnAdd(uint32_t pageAdd)
{
  if(pageAdd > w25nGetMaxPage()) W25N_ERROR(); //return 1;
  return W25nDieSelect(pageAdd / W25N01GV_MAX_PAGE);
}

int W25nDieSelect(char die)
{
  //TODO add some type of input validation
  char buf[2] = {W25M_DIE_SELECT, die};
  w25nSendData(buf, sizeof(buf));
  _dieSelect = die;
  return 0;
}


void w25nWriteEnable()
{
  char buf[] = {W25N_WRITE_ENABLE};
  w25nSendData(buf, sizeof(buf));
}

void w25nWriteDisable()
{
  char buf[] = {W25N_WRITE_DISABLE};
  w25nSendData(buf, sizeof(buf));
}

int w25nBlockErase(uint32_t pageAdd)
{
  if(pageAdd > w25nGetMaxPage()) W25N_ERROR(); //return 1;
  w25nDieSelectOnAdd(pageAdd);
  char pageHigh = (char)((pageAdd & 0xFF00) >> 8);
  char pageLow = (char)(pageAdd);
  char buf[4] = {W25N_BLOCK_ERASE, 0x00, pageHigh, pageLow};
  w25nBlock_WIP();
  w25nWriteEnable();
  w25nSendData(buf, sizeof(buf));
  HAL_Delay(10);
  return 0;
}

int w25nBulkErase()
{
  int error = 0;
  for(uint32_t i = 0; i < w25nGetMaxPage(); i+=64){
    if((error = w25nBlockErase(i)) != 0) W25N_ERROR(); //return 1;
  }
  return 0;
}

int w25nLoadProgData(uint16_t columnAdd, char* buf, uint32_t dataLen)
{
    if(columnAdd > (uint32_t)W25N_MAX_COLUMN) W25N_ERROR(); //return 1;
    if(dataLen > (uint32_t)W25N_MAX_COLUMN - columnAdd) W25N_ERROR(); //return 1;
    char columnHigh = (columnAdd & 0xFF00) >> 8;
    char columnLow = columnAdd & 0xff;
    char cmdbuf[3] = {W25N_PROG_DATA_LOAD, columnHigh, columnLow};
    w25nBlock_WIP();
    w25nWriteEnable();
    digitalWrite(_GPIOx, _cs, LOW);
    HAL_SPI_TransmitReceive(&_hspi, (uint8_t*)cmdbuf, (uint8_t*)cmdbuf, sizeof(cmdbuf), 100);
    HAL_SPI_TransmitReceive(&_hspi, (uint8_t*)buf, (uint8_t*)buf, dataLen, 100);
    digitalWrite(_GPIOx, _cs, HIGH);
    return 0;
}

// int w25nLoadProgData0(uint16_t columnAdd, char* buf, uint32_t dataLen, uint32_t pageAdd)
// {
//   if(w25nDieSelectOnAdd(pageAdd)) W25N_ERROR(); //return 1;
//   return w25nLoadProgData(columnAdd, buf, dataLen);
// }

int w25nLoadRandProgData(uint16_t columnAdd, char* buf, uint32_t dataLen)
{
    if(columnAdd > (uint32_t)W25N_MAX_COLUMN) W25N_ERROR(); //return 1;
    if(dataLen > (uint32_t)W25N_MAX_COLUMN - columnAdd) W25N_ERROR(); //return 1;
    char columnHigh = (columnAdd & 0xFF00) >> 8;
    char columnLow = columnAdd & 0xff;
    char cmdbuf[3] = {W25N_RAND_PROG_DATA_LOAD, columnHigh, columnLow};
    w25nBlock_WIP();
    w25nWriteEnable();
    digitalWrite(_GPIOx, _cs, LOW);
    HAL_SPI_TransmitReceive(&_hspi, (uint8_t*)cmdbuf, (uint8_t*)cmdbuf, sizeof(cmdbuf), 100);
    HAL_SPI_TransmitReceive(&_hspi, (uint8_t*)buf, (uint8_t*)buf, dataLen, 100);
    digitalWrite(_GPIOx, _cs, HIGH);
    return 0;
}

// int w25nLoadRandProgData0(uint16_t columnAdd, char* buf, uint32_t dataLen, uint32_t pageAdd)
// {
//   if(w25nDieSelectOnAdd(pageAdd)) W25N_ERROR(); //return 1;
//   return w25nLoadRandProgData(columnAdd, buf, dataLen);
// }

int w25nProgramExecute(uint32_t pageAdd)
{
  if(pageAdd > w25nGetMaxPage()) W25N_ERROR(); //return 1;
  w25nDieSelectOnAdd(pageAdd);
  char pageHigh = (char)((pageAdd & 0xFF00) >> 8);
  char pageLow = (char)(pageAdd);
  w25nWriteEnable();
  char buf[4] = {W25N_PROG_EXECUTE, 0x00, pageHigh, pageLow};
  w25nSendData(buf, sizeof(buf));
  return 0;
}

int w25nPageDataRead(uint32_t pageAdd)
{
  if(pageAdd > w25nGetMaxPage()) W25N_ERROR(); //return 1;
  w25nDieSelectOnAdd(pageAdd);
  char pageHigh = (char)((pageAdd & 0xFF00) >> 8);
  char pageLow = (char)(pageAdd);
  char buf[4] = {W25N_PAGE_DATA_READ, 0x00, pageHigh, pageLow};
  w25nBlock_WIP();
  w25nSendData(buf, sizeof(buf));
  return 0;

}

int w25nRead(uint16_t columnAdd, char* buf, uint32_t dataLen)
{
    if(columnAdd > (uint32_t)W25N_MAX_COLUMN) W25N_ERROR(); //return 1;
    if(dataLen > (uint32_t)W25N_MAX_COLUMN - columnAdd) W25N_ERROR(); //return 1;
    char columnHigh = (columnAdd & 0xFF00) >> 8;
    char columnLow = columnAdd & 0xff;
    char cmdbuf[4] = {W25N_READ, columnHigh, columnLow, 0x00};
    w25nBlock_WIP();
    digitalWrite(_GPIOx, _cs, LOW);
    HAL_SPI_TransmitReceive(&_hspi, (uint8_t*)cmdbuf, (uint8_t*)cmdbuf, sizeof(cmdbuf), 100);
    HAL_SPI_TransmitReceive(&_hspi, (uint8_t*)buf, (uint8_t*)buf, dataLen, 100);
    digitalWrite(_GPIOx, _cs, HIGH);
    return 0;
}

int w25nCheck_WIP()
{
  char status = w25nGetStatusReg(W25N_STAT_REG);
  if(status & 0x01){
    return 1;
  }
  return 0;
}                      

int w25nBlock_WIP()
{
  while(w25nCheck_WIP()){
    HAL_Delay(15);
  }
  return 0;
}

int w25nCheck_status()
{
  return(w25nGetStatusReg(W25N_STAT_REG));
}
