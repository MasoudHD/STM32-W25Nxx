#include "w25n.h"

//Change these handlers based on yours
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

char* hello = "Hello world, lets test NAND Flash";
char* data1 = "This is First data to write";
char* data2 = "This is Second data to write";

char buf[512];

void println(char* str)
{
    //Change huart1 based on your USART handler
    HAL_UART_Transmit(&huart1, (unsigned char*)str, strlen(str), 100);
    HAL_UART_Transmit(&huart1, (unsigned char*)"\n\r", 2, 100); 
}

void InitProgram(void)
{
  //Init peripherals you use (SPI, USART, ...)
}

int main(void)
{
  InitProgram();

  println("Start testing W25Nxx NAND Flash...");     
  memset(buf, 0, sizeof(buf));

  //Write your chip select port and pin and also the SPI
  if(w25nInit(GPIOB, GPIO_PIN_15, &hspi1) == 0)
  {
      println("Flash init successful");
  }
  else
  {
      println("Flash init Failed");
  }

  if(w25nGetModel() == W25M02GV)
  {
    println("Model: W25M02GV");
  }
  else if(w25nGetModel() == W25N01GV)
  {
    println("Model: W25N01GV");
  }
  else
  {
    println("Model: UNKNOWN");
  }

  //Erase First Block (64 pages => 64*2048 = 131072Bytes)  
  w25nBlockErase(0);

  //Writing some data 
  memcpy(buf, hello, strlen(hello) + 1);
  w25nLoadRandProgData(0, buf, strlen(hello) + 1);
  //Some more...
  memcpy(buf, data1, strlen(data1) + 1);
  w25nLoadRandProgData(strlen(hello) + 1, buf, strlen(data1) + 1);
  //And one more
  memcpy(buf, data2, strlen(data2) + 1);
  w25nLoadRandProgData(strlen(hello) + strlen(data1) + 2, buf, strlen(data2) + 1);

  w25nProgramExecute(0);


  //Reading data
  w25nPageDataRead(0);

  memset(buf, 0, sizeof(buf));
  w25nRead(0, buf, strlen(hello));
  println(buf);

  memset(buf, 0, sizeof(buf));
  w25nRead(strlen(hello)+1, buf, strlen(data1));
  println(buf);

  memset(buf, 0, sizeof(buf));
  w25nRead(strlen(hello) + strlen(data1) + 2, buf, strlen(data2));
  println(buf);

  while (1)
  {

  }

  return 0; 
}