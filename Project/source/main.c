#include "STC15F2K60S2.H"
#include "sys.H"
#include "uart1.h"
#include "displayer.h"
#include "Vib.h"
#include "Key.H"
#include "adc.h"

code unsigned long SysClock=11059200;

code char decode_table[] = {
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,
  0x00,0x6d,0x1c,0x54,0x39,0x38,0x5c,0x5e,0x50,0x77,0x04,0x79,0x80,0x40};
/*
  10 (empty)
  11 S
  12 u
  13 n
  14 C
  15 L
  16 o
  17 d
  18 r
  19 A
  20 i
  21 E
  22 .
  23 -
  */

code char weat2code_table[6][5] = {
  {11, 12, 13, 10, 10}, // Sun
  {14, 15, 16, 12, 17}, // CLoud
  {18, 19, 20, 13, 10}, // rAin
  {11, 13, 16, 12, 10}, // Snou
  {21, 18, 18, 16, 18}, // Error
  {22, 22, 22, 22, 22}  // ..... (waiting for sync)
};

struct_ADC adc_data;
code int tempdata[]={239,197,175,160,150,142,135,129,124,120,116,113,109,107,104,101,99,97,95,93,91,90,88,86,85,84,82,81,80,78,77,76,75,74,73,72,71,70,69,68,67,67,66,65,64,63,63,62,61,61,60,59,58,58,57,57,56,55,55,54,54,53,52,52,51,51,50,50,49,49,48,48,47,47,46,46,45,45,44,44,43,43,42,42,41,41,41,40,40,39,39,38,38,38,37,37,36,36,36,35,35,34,34,34,33,33,32,32,32,31,31,31,30,30,29,29,29,28,28,28,27,27,27,26,26,26,25,25,24,24,24,23,23,23,22,22,22,21,21,21,20,20,20,19,19,19,18,18,18,17,17,16,16,16,15,15,15,14,14,14,13,13,13,12,12,12,11,11,11,10,10,9,9,9,8,8,8,7,7,7,6,6,5,5,54,4,3,3,3,2,2,1,1,1,0,0,-1,-1,-1,-2,-2,-3,-3,-4,-4,-5,-5,-6,-6,-7,-7,-8,-8,-9,-9,-10,-10,-11,-11,-12,-13,-13,-14,-14,-15,-16,-16,-17,-18,-19,-19,-20,-21,-22,-23,-24,-25,-26,-27,-28,-29,-30,-32,-33,-35,-36,-38,-40,-43,-46,-50,-55,-63,361};
unsigned int t=0, sumt=0;

char matchhead[2]={0xaa,0x55};
char rxd[5],ack[5];

char buf[3][8] = {
  {22,22,22,22,22,10,22,22},
  {10,10,10,10,10,10,22,22},
  {10,10,10,10,10,10,10,22}
};

char req[5] = {0xaa,0x55,0xff,0xff,0xff};

char show = 0;

void uart1rxd_callback();
void sys100ms_callback();
void sys1ms_callback();
void vib_callback();
void key_callback();
void nav_callback();

void main()
{
  VibInit();
  Uart1Init(2400);
  DisplayerInit();
  SetDisplayerArea(0,7);
  KeyInit();
  AdcInit(ADCexpEXT);
  
  SetUart1Rxd(rxd, 5, matchhead, 2);
  SetEventCallBack(enumEventUart1Rxd, uart1rxd_callback);
  SetEventCallBack(enumEventSys100mS, sys100ms_callback);
  SetEventCallBack(enumEventSys1mS, sys1ms_callback);
  SetEventCallBack(enumEventVib, vib_callback);
  SetEventCallBack(enumEventKey, key_callback);
  SetEventCallBack(enumEventNav, nav_callback);
  
  MySTC_Init();
  while (1) MySTC_OS();
}

void set_buf_error(){
  int i;
  for (i=0;i<5;i++) buf[0][i] = weat2code_table[3][i];
  for (i=5;i<8;i++) buf[0][i] = 10;
}

void set_buf_weat(char weat){
  int i;
  if (weat > 3){
    set_buf_error();
    return;
  }
  for (i=0;i<5;i++) buf[0][i] = weat2code_table[weat][i];
}

void set_buf_temp(int target, signed char temp){
  if (temp < 0) buf[target][5] = 23, temp=-temp;
  buf[target][6] = temp/10 % 10;
  buf[target][7] = temp % 10;
}

void update_data(){
  Uart1Print(req, 5); // send request
}

void set_buf_rg(char f){ // rain gear
  buf[2][7] = f?18:16;
}

void uart1rxd_callback(){
  //int i;
  //for (i=0;i<5;i++) ack[i] = rxd[i];

  set_buf_weat(rxd[2]);
  set_buf_temp(0,rxd[3]);
  
  set_buf_rg(rxd[2] == 2 || rxd[2] == 3);
  
  //Uart1Print(ack, 5);
}

void sys100ms_callback(){
  Seg7Print(buf[show][0],buf[show][1],buf[show][2],buf[show][3],buf[show][4],buf[show][5],buf[show][6],buf[show][7]);
}

void sys1ms_callback(){
  t++;
  if (t == 200){
    set_buf_temp(1, tempdata[(sumt+t/2)/t - 1]);
    sumt = t = 0;
  }
  adc_data = GetADC();
  sumt += adc_data.Rt >> 2;
}

void vib_callback(){
  char vib_act = GetVibAct();
  if (vib_act == enumVibQuake) update_data();
}

void key_callback(){
  if (GetKeyAct(enumKey1) == enumKeyRelease){
    show = 0;
  }
  if (GetKeyAct(enumKey2) == enumKeyRelease){
    show = 1;
  }
  // When ADC enabled, K3 is managed by ADC module
//  if (GetKeyAct(enumKey3) == enumKeyRelease){
//    show = 2;
//  }
}

void nav_callback(){
  if (GetAdcNavAct(enumAdcNavKey3) == enumKeyRelease){
    show = 2;
  }
}