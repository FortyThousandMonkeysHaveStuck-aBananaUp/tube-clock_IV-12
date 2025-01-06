#define STM32F103xB
#include "stm32f1xx.h"

//definitions
#define ALARM_ON 1
#define ALARM_OFF 0
#define TIME_WAS_NOT_UPDATED 1
#define TIME_WAS_UPDATED 0
#define LIGHT_ON 1      //for blinking
#define LIGHT_OFF 0     //for blinking
#define CLK_UP          GPIOA->BSRR|=GPIO_BSRR_BS1
#define CLK_DOWN        GPIOA->BSRR|=GPIO_BSRR_BR1
#define DS_UP           GPIOA->BSRR|=GPIO_BSRR_BS2
#define DS_DOWN         GPIOA->BSRR|=GPIO_BSRR_BR2
#define LATCH_UP        GPIOA->BSRR|=GPIO_BSRR_BS3
#define LATCH_DOWN      GPIOA->BSRR|=GPIO_BSRR_BR3
#define RESET_UP        GPIOA->BSRR|=GPIO_BSRR_BS4
#define RESET_DOWN      GPIOA->BSRR|=GPIO_BSRR_BR4
#define RISING_CHECK_EDGE GPIOB->BSRR|=GPIO_BSRR_BS11
#define FALLING_CHECK_EDGE GPIOB->BSRR|=GPIO_BSRR_BR11
#define RISING_SIGNAL_EDGE GPIOB->BSRR|=GPIO_BSRR_BS9
#define FALLING_SIGNAL_EDGE GPIOB->BSRR|=GPIO_BSRR_BR9
#define IS_THE_BUTTON_1_PRESSED GPIOB->IDR&GPIO_IDR_IDR15
#define IS_THE_BUTTON_2_PRESSED GPIOB->IDR&GPIO_IDR_IDR14
#define IS_THE_BUTTON_3_PRESSED GPIOB->IDR&GPIO_IDR_IDR13
#define IS_THE_BUTTON_4_PRESSED GPIOB->IDR&GPIO_IDR_IDR12

//Delays:
#define HALF_SECOND_DELAY 1565500        //1565500 for half second
#define TWO_SECOND_DELAY 6262000        //3131000 for one second

//Varaibles
char lamp_time_was_updated=TIME_WAS_NOT_UPDATED;
char alarm_on_off=ALARM_OFF;

const char rotation_array[6]={0x80,0x04,0x08,0x10,0x20,0x40};

const char numbers_array[16]={
  0xFC, //"0"
  0x30, //"1"
  0x6E, //"2"
  0x7A, //"3"
  0xB2, //"4"
  0xDA, //"5"
  0xDE, //"6"
  0x70, //"7"
  0xFE, //"8"
  0xFA, //"9"
  0xF6, //"10"
  0x9E, //"11"
  0xCC, //"12"
  0x3E, //"13"
  0xCE, //"14"
  0xC6  //"15"
};

typedef struct{
char discharge_selection;
char Tens_of_hours, Units_of_hours;//for time
char Tens_of_minutes, Units_of_minutes;//for time
char Alarm_Tens_of_hours, Alarm_Units_of_hours;//for alarm
char Alarm_Tens_of_minutes, Alarm_Units_of_minutes;//for alarm
}RTC_structure;

RTC_structure RTC_buffer={.discharge_selection=1};

//Function declarations
void System_Clock_Config(void);
void GPIO_PB12_15_and_PB9_and_PB11_Config(void);
void GPIO_PA1_5_Config(void);
void RTC_Config(void);
void prepare_pins(void);
void RTC_Set_Time(RTC_structure * RTC_buffer_pointer);
void RTC_Set_Alarm(RTC_structure * RTC_buffer_pointer);
void numbers_selection_signal(void);
void delay_without_interruption(unsigned long wait);
void effect_of_rotation(void);
void effect_hello(void);
void effect_Fto0(void);
void effect_0toF(void);
void write_time_register(unsigned int value);
unsigned int time_shaper(RTC_structure * RTC_buffer_pointer);
void numbers_selection(RTC_structure * RTC_buffer_pointer);
void menu_selection_mode(RTC_structure * RTC_buffer_pointer);
void duration_test(void);

//Interruption
void RTC_IRQHandler(void)
{
  if(RTC->CRL&RTC_CRL_SECF)
  {
    RTC->CRL&=~RTC_CRL_SECF;
    
    //PB11 for duration checking
    if(GPIOB->IDR&~GPIO_IDR_IDR11)       {RISING_CHECK_EDGE;}//To check the duration of one second
    //else                                {FALLING_CHECK_EDGE;}
    
    unsigned int all_seconds=((RTC->CNTH<<16)+RTC->CNTL);//There is 86400 seconds in 24 hours
    if(all_seconds>=86400)//set 00:00 when time is 24 hours
    {
    while(all_seconds>=86400){all_seconds-=86400;}
    
      //Steps
      //1 Poll the RTOFF bit
      //2 Set the CNF bit
      //3 Set CNTH=0; and CNTL=0;
      //4 Reset the CNF bit
      //5 Poll the RTOFF bit
    
      //1 Poll the RTOFF bit
      while(!(RTC->CRL&RTC_CRL_RTOFF));
      
      //2 Set the CNF bit
      RTC->CRL|=RTC_CRL_CNF;
      
      //3 Set CNTH=0; and CNTL=0;
      RTC->CNTH=all_seconds>>16;
      RTC->CNTL=all_seconds&0x0000FFFF;
      
      //4 Reset the CNF bit
      RTC->CRL&=~RTC_CRL_CNF;
      
      //5 Poll the RTOFF bit
      while(!(RTC->CRL&RTC_CRL_RTOFF));
    }
    
    if(((RTC->CNTH<<16)+RTC->CNTL)%60==0)
    {lamp_time_was_updated=TIME_WAS_NOT_UPDATED;}
  }
  if((RTC->CRL&RTC_CRL_ALRF)&&(RTC->CRH&RTC_CRH_ALRIE))
  {
    RTC->CRH&=~RTC_CRH_ALRIE;   //Disable the interruption from an alarm module
    RTC->CRL&=~RTC_CRL_ALRF;    //Reset the interruption flag
    alarm_on_off=ALARM_ON;
  }
}

void main(void)
{
  //Config and effect
  System_Clock_Config();
  GPIO_PB12_15_and_PB9_and_PB11_Config();
  GPIO_PA1_5_Config();
  RTC_Config();
  
  prepare_pins();
  
  effect_of_rotation();
  lamp_time_was_updated=TIME_WAS_NOT_UPDATED;
  
  while(2)
  {     
    FALLING_CHECK_EDGE;//PB11 for duration checking
  
    if(alarm_on_off==ALARM_ON)
    {
      numbers_selection_signal();//PB9
      delay_without_interruption(80);
      
      numbers_selection_signal();
      delay_without_interruption(80);
    }
    if(lamp_time_was_updated==TIME_WAS_NOT_UPDATED)
    {
      lamp_time_was_updated=TIME_WAS_UPDATED;
      
      unsigned int all_seconds=(RTC->CNTH<<16)+RTC->CNTL;
      
      RTC_buffer.Tens_of_hours=(all_seconds/3600)/10;
      RTC_buffer.Units_of_hours=(all_seconds/3600)%10;
      RTC_buffer.Tens_of_minutes=((all_seconds-(all_seconds/3600)*3600)/60)/10;
      RTC_buffer.Units_of_minutes=((all_seconds-(all_seconds/3600)*3600)/60)%10;
      
      write_time_register(time_shaper(&RTC_buffer));
    }
    //Handling of button presses
    if(IS_THE_BUTTON_1_PRESSED)
    {
      effect_hello();
      lamp_time_was_updated=TIME_WAS_NOT_UPDATED;
    }
      else if(IS_THE_BUTTON_2_PRESSED)
      {
        effect_0toF();
        lamp_time_was_updated=TIME_WAS_NOT_UPDATED;
      }
        else if(IS_THE_BUTTON_3_PRESSED)
        {
          effect_Fto0();
          lamp_time_was_updated=TIME_WAS_NOT_UPDATED;
        }
          else if(IS_THE_BUTTON_4_PRESSED)
          {
            menu_selection_mode(&RTC_buffer);
            lamp_time_was_updated=TIME_WAS_NOT_UPDATED;
          }
  }
}

//Function bodies
//##---------------------------------------------------------------------------##//System_Clock_Config
void System_Clock_Config(void)
{
  //Steps
  //1 Latency
  //2 Enable HSE and wait for
  //3 PLL multiplication factor
  //4 PLL entry clock source
  //5 Enable PLL and wait for
  //6 Set the prescalers
    //HPRE
    //PPRE1 is divided by 2
    //PPRE2
  //7 SW and wait for

  //1 Latency
  FLASH->ACR|=FLASH_ACR_LATENCY_1;
  
  //2 Enable HSE and wait for
  RCC->CR|=RCC_CR_HSEON;
  while(!(RCC->CR&RCC_CR_HSERDY));
  
  //3 PLL multiplication factor
  RCC->CFGR|=RCC_CFGR_PLLMULL9;
  
  //4 PLL entry clock source
  RCC->CFGR|=RCC_CFGR_PLLSRC;
  
  //5 Enable PLL and wait for
  RCC->CR|=RCC_CR_PLLON;
  while(!(RCC->CR&RCC_CR_PLLRDY));
  
  //6 Set the prescalers
    //HPRE
    RCC->CFGR&=~RCC_CFGR_HPRE;
    
    //PPRE1 is divided by 2
    RCC->CFGR&=~RCC_CFGR_PPRE1;//set the PPRE1 to zero
    RCC->CFGR|=RCC_CFGR_PPRE1_DIV2;
    
    //PPRE2
    RCC->CFGR&=~RCC_CFGR_PPRE2;
    
  //7 SW and wait for
  RCC->CFGR|=RCC_CFGR_SW_PLL;
  while(!(RCC->CFGR&RCC_CFGR_SWS_PLL));
}

//##---------------------------------------------------------------------------##//GPIO_PB12_15_and_PB9_and_PB11_Config
void GPIO_PB12_15_and_PB9_and_PB11_Config(void)
{
  //Steps
  //1 Clock
  //2 Reset
  //3 Mode
  //4 Configurate

  //1 Clock
  RCC->APB2ENR|=RCC_APB2ENR_IOPBEN;
  
  //2 Reset
  GPIOB->BRR=0xFFFF;//Reset all pins
  
  //3 Mode
  GPIOB->CRH&=~GPIO_CRH_MODE12;//00 for input
  GPIOB->CRH&=~GPIO_CRH_MODE13;//00 for input
  GPIOB->CRH&=~GPIO_CRH_MODE14;//00 for input
  GPIOB->CRH&=~GPIO_CRH_MODE15;//00 for input
        
  GPIOB->CRH|=GPIO_CRH_MODE9;//11 for output
  GPIOB->CRH|=GPIO_CRH_MODE11;//11 for output 
  
  //4 Configurate
  GPIOB->CRH&=~GPIO_CRH_CNF12;//set to zero
  GPIOB->CRH&=~GPIO_CRH_CNF13;//set to zero
  GPIOB->CRH&=~GPIO_CRH_CNF14;//set to zero
  GPIOB->CRH&=~GPIO_CRH_CNF15;//set to zero
  
  GPIOB->CRH|=(0x2<<18);//CNF12 10: Input with pull-up / pull-down
  GPIOB->CRH|=(0x2<<22);//CNF13 10: Input with pull-up / pull-down
  GPIOB->CRH|=(0x2<<26);//CNF14 10: Input with pull-up / pull-down
  GPIOB->CRH|=(0x2<<30);//CNF15 10: Input with pull-up / pull-down
  
  GPIOB->CRH&=~GPIO_CRH_CNF9;//00: General purpose output push-pull
  GPIOB->CRH&=~GPIO_CRH_CNF11;//00: General purpose output push-pull
}

//##---------------------------------------------------------------------------##//GPIO_PA1_5_Config
void GPIO_PA1_5_Config(void)
{
  //Steps
  //1 Clock
  //2 Reset
  //3 Mode
  //4 Configurate

  //1 Clock
  RCC->APB2ENR|=RCC_APB2ENR_IOPAEN;
  
  //2 Reset
  GPIOA->BRR=0xFFFF;//reset the pins
  
  //3 Mode
  GPIOA->CRL|=GPIO_CRL_MODE1;//11 for output
  GPIOA->CRL|=GPIO_CRL_MODE2;//11 for output
  GPIOA->CRL|=GPIO_CRL_MODE3;//11 for output
  GPIOA->CRL|=GPIO_CRL_MODE4;//11 for output  
  GPIOA->CRL|=GPIO_CRL_MODE5;//11 for output
  
  //4 Configurate
  GPIOA->CRL&=~GPIO_CRL_CNF1;//00: General purpose output push-pull
  GPIOA->CRL&=~GPIO_CRL_CNF2;//00: General purpose output push-pull
  GPIOA->CRL&=~GPIO_CRL_CNF3;//00: General purpose output push-pull
  GPIOA->CRL&=~GPIO_CRL_CNF4;//00: General purpose output push-pull
  GPIOA->CRL&=~GPIO_CRL_CNF5;//00: General purpose output push-pull_____________________________________________________Do I use this 5th pin anywhere?
}

//##---------------------------------------------------------------------------##//RTC_Config
void RTC_Config(void)
{
  //Steps
  //1 RCC->APB1ENR PWREN=1
  //2 PWR->CR DBP=1
  //3 Reset the entire backup register
  //4 RTC->BDCR RTCSEL=1
  //5 LSEON=1 LSERDY
  //6 RTCEN=1
  //7 Configuration procedure
    //7.1 Poll the RTOFF bit
    //7.2 Set the CNF bit
    //7.3 Write something somewhere
    //7.4 Reset the CNF bit
    //7.5 Poll the RTOFF bit
  //8 Interrupts

  //1 RCC->APB1ENR PWREN=1
  RCC->APB1ENR|=RCC_APB1ENR_PWREN;
  
  //2 PWR->CR DBP=1
  PWR->CR|=PWR_CR_DBP;
  
  //3 Reset the entire backup register// It will reset all time!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  RCC->BDCR|=RCC_BDCR_BDRST;
//  RCC->BDCR&=~RCC_BDCR_BDRST;
  
  //4 RCC->BDCR RTCSEL=1
  RCC->BDCR&=~RCC_BDCR_RTCSEL;
  RCC->BDCR|=(0x1<<8);
  
  //5 LSEON=1 LSERDY
  RCC->BDCR|=RCC_BDCR_LSEON;
  while(!(RCC->BDCR&RCC_BDCR_LSERDY));
  
  //6 RTCEN=1
  RCC->BDCR|=RCC_BDCR_RTCEN;
  
  //7 Configuration procedure
    //7.1 Poll the RTOFF bit
    while(!(RTC->CRL&RTC_CRL_RTOFF));
    
    //7.2 Set the CNF bit
    RTC->CRL|=RTC_CRL_CNF;
    
    //7.3 Write something somewhere
    RTC->PRLH=0;
    RTC->PRLL=0x7FFB;
    //7FFA -33us


    //7.4 Reset the CNF bit
    RTC->CRL&=~RTC_CRL_CNF;
    
    //7.5 Poll the RTOFF bit
    while(!(RTC->CRL&RTC_CRL_RTOFF));
    
  //8 Interrupts
  RTC->CRL&=~RTC_CRL_SECF;//Reset the flag
  RTC->CRL&=~RTC_CRL_ALRF;//Reset the flag
  RTC->CRH|=RTC_CRH_SECIE;//Enable interruption
  RTC->CRH&=~RTC_CRH_ALRIE;//Disable interruption from an alarm module
  NVIC_SetPriority(RTC_IRQn, 1);
  NVIC_EnableIRQ(RTC_IRQn);
}

//##---------------------------------------------------------------------------##//prepare_pins
void prepare_pins(void)
{
  CLK_DOWN;
  DS_DOWN;
  RESET_DOWN;
  LATCH_DOWN;
  
  RESET_UP;
  DS_UP;
  
  RESET_DOWN;
  LATCH_UP;
  LATCH_DOWN;
  RESET_UP;
}

//##---------------------------------------------------------------------------##//RTC_Set_Time
void RTC_Set_Time(RTC_structure * RTC_buffer_pointer)
{
  unsigned int all_seconds=(RTC_buffer_pointer->Tens_of_hours*10+RTC_buffer_pointer->Units_of_hours)*3600+//hours to seconds
                            (RTC_buffer_pointer->Tens_of_minutes*10+RTC_buffer_pointer->Units_of_minutes)*60;//minutes to seconds
  
  while(!(RTC->CRL&RTC_CRL_RTOFF));
  RTC->CRL|=RTC_CRL_CNF;
  
  RTC->CNTH=all_seconds>>16;
  RTC->CNTL=all_seconds&0x0000FFFF;
  
  RTC->CRL&=~RTC_CRL_CNF;
  while(!(RTC->CRL&RTC_CRL_RTOFF));
}

//##---------------------------------------------------------------------------##//RTC_Set_Alarm
void RTC_Set_Alarm(RTC_structure * RTC_buffer_pointer)
{//and interruptions
  unsigned int all_seconds=(RTC_buffer_pointer->Alarm_Tens_of_hours*10+RTC_buffer_pointer->Alarm_Units_of_hours)*3600+//hours to seconds
                        (RTC_buffer_pointer->Alarm_Tens_of_minutes*10+RTC_buffer_pointer->Alarm_Units_of_minutes)*60;//minutes to seconds
  
  while(!(RTC->CRL&RTC_CRL_RTOFF));
  RTC->CRL|=RTC_CRL_CNF;
  
  RTC->ALRH=all_seconds>>16;
  RTC->ALRL=all_seconds&0x0000FFFF;
  
  RTC->CRL&=~RTC_CRL_CNF;
  while(!(RTC->CRL&RTC_CRL_RTOFF));
  
  //and interruptions
  RTC->CRL&=~RTC_CRL_ALRF;
  RTC->CRH|=RTC_CRH_ALRIE;
  NVIC_SetPriority(RTC_IRQn, 1);
  NVIC_EnableIRQ(RTC_IRQn);
}

//##---------------------------------------------------------------------------##//numbers_selection_signal
void numbers_selection_signal(void)
{
  RISING_SIGNAL_EDGE;
  delay_without_interruption(80);
  
  FALLING_SIGNAL_EDGE;
  delay_without_interruption(80);
}

//##---------------------------------------------------------------------------##//delay_without_interruption
void delay_without_interruption(unsigned long wait)
{
  wait*=10300;
  while(--wait);
}

//##---------------------------------------------------------------------------##//effect_of_rotation
void effect_of_rotation(void)
{
  unsigned int buffer=0;
  
  for(char segment=0; segment<6; segment++)
  {
    buffer|=rotation_array[segment];
    buffer<<=24;
    write_time_register(buffer);
    buffer>>=24;
    delay_without_interruption(60);
  }
  delay_without_interruption(80);
  
  buffer<<=8;
  for(char segment=0; segment<6; segment++)
  {
    buffer|=rotation_array[segment];
    buffer<<=16;
    write_time_register(buffer);
    buffer>>=16;
    delay_without_interruption(60);
  }
  delay_without_interruption(80);
  
  buffer<<=8;
  for(char segment=0; segment<6; segment++)
  {
    buffer|=rotation_array[segment];
    buffer<<=8;
    write_time_register(buffer);
    buffer>>=8;
    delay_without_interruption(60);
  }
  delay_without_interruption(80);
  
  buffer<<=8;
  for(char segment=0; segment<6; segment++)
  {
    buffer|=rotation_array[segment];
    write_time_register(buffer);
    delay_without_interruption(60);
  }
  delay_without_interruption(80);
}

//##---------------------------------------------------------------------------##//effect_hello
void effect_hello(void)
{
  for(char repetitions=0; repetitions<4; repetitions++)
  {
    write_time_register(0xB6CEB4FC);
    delay_without_interruption(250);
    
    write_time_register(0x00);
    delay_without_interruption(450);
  }
}

//##---------------------------------------------------------------------------##//effect_Fto0
void effect_Fto0(void)
{
  for(char number=16; number!=0; number--)
  {
    unsigned int buffer=0;
    
    for(char discharge=0; discharge<4; discharge++)
    {
      buffer|=numbers_array[number-1];
      if(discharge<(4-1)){buffer<<=8;}//Do not move in last time
    }
    
    write_time_register(buffer);
    delay_without_interruption(160);
    
    write_time_register(0x00);
    delay_without_interruption(160);
  }
}

//##---------------------------------------------------------------------------##//effect_0toF
void effect_0toF(void)
{
  for(char number=0; number<16; number++)
  {
    unsigned int buffer=0;
    
    for(char discharge=0; discharge<4; discharge++)
    {
      buffer|=numbers_array[number];
      if(discharge<(4-1)){buffer<<=8;}//Do not move in last time
    }
    
    write_time_register(buffer);
    delay_without_interruption(160);
    
    write_time_register(0x00);
    delay_without_interruption(160);
  }
}

//##---------------------------------------------------------------------------##//write_time_register
void write_time_register(unsigned int value)
{
  for(char i=0; i<32; i++)
  {
    if(value&0x0001)    {DS_UP;}
    else                {DS_DOWN;}
    
    CLK_UP;
    CLK_DOWN;
    
    value>>=1;
  }
  
  LATCH_UP;
  LATCH_DOWN;
}

//##---------------------------------------------------------------------------##//time_shaper
unsigned int time_shaper(RTC_structure * RTC_buffer_pointer)
{
  unsigned int buffer=0;
  
  //HOURS
  buffer|=numbers_array[RTC_buffer_pointer->Tens_of_hours];
  buffer<<=8;
  buffer|=numbers_array[RTC_buffer_pointer->Units_of_hours];
  buffer<<=8;
  
  //MINUTES
  buffer|=numbers_array[RTC_buffer_pointer->Tens_of_minutes];
  buffer<<=8;
  buffer|=numbers_array[RTC_buffer_pointer->Units_of_minutes];
  
  //SECONDS
  //there is nothing
  
  return buffer;
}

//##---------------------------------------------------------------------------##//number_selection
void numbers_selection(RTC_structure * RTC_buffer_pointer)
{
  unsigned int buffer=0;
  char blink_on_off=LIGHT_ON;

  while(IS_THE_BUTTON_4_PRESSED){;}
  delay_without_interruption(250);
  
  while(!(IS_THE_BUTTON_4_PRESSED))//press the button 4 to exit
  {
    //blink without interruption
    buffer=time_shaper(RTC_buffer_pointer);
    
    if(blink_on_off==LIGHT_ON)
    {
      blink_on_off=LIGHT_OFF;
      write_time_register(buffer);
      delay_without_interruption(50);
    }else if(blink_on_off==LIGHT_OFF)
    {
      blink_on_off=LIGHT_ON;
      switch(RTC_buffer_pointer->discharge_selection)
      {
        case 1:   {write_time_register(buffer&0x00FFFFFF); break;}
        case 2:   {write_time_register(buffer&0xFF00FFFF); break;}
        case 3:   {write_time_register(buffer&0xFFFF00FF); break;}
        case 4:   {write_time_register(buffer&0xFFFFFF00); break;}
        default: {;}
      }
      delay_without_interruption(100);
    }
    //blink without interruption_the end
    
  
    if(IS_THE_BUTTON_1_PRESSED)//for increase the number by one
    {
      while(IS_THE_BUTTON_1_PRESSED){;}
      delay_without_interruption(250);
      
      if(RTC_buffer_pointer->discharge_selection==1)
      {
        if(RTC_buffer_pointer->Tens_of_hours<2)
        {RTC_buffer_pointer->Tens_of_hours++;}
        if((RTC_buffer_pointer->Tens_of_hours==2)&&(RTC_buffer_pointer->Units_of_hours>3))
        {RTC_buffer_pointer->Units_of_hours=0;}
      }else if(RTC_buffer_pointer->discharge_selection==2)
      {
        if((RTC_buffer_pointer->Tens_of_hours==2)&&(RTC_buffer_pointer->Units_of_hours<3))
        {RTC_buffer_pointer->Units_of_hours++;}
        if((RTC_buffer_pointer->Tens_of_hours<2)&&(RTC_buffer_pointer->Units_of_hours<9))
        {RTC_buffer_pointer->Units_of_hours++;}
      }else if(RTC_buffer_pointer->discharge_selection==3)
      {
        if(RTC_buffer_pointer->Tens_of_minutes<5)
        {RTC_buffer_pointer->Tens_of_minutes++;}
      }else if(RTC_buffer_pointer->discharge_selection==4)
      {
        if(RTC_buffer_pointer->Units_of_minutes<9)
        {RTC_buffer_pointer->Units_of_minutes++;}
      }
    }
    if(IS_THE_BUTTON_2_PRESSED)//for decrease the number by one
    {
      while(IS_THE_BUTTON_2_PRESSED){;}
      delay_without_interruption(250);
      
      if(RTC_buffer_pointer->discharge_selection==1)
      {
        if(RTC_buffer_pointer->Tens_of_hours>0)
        {RTC_buffer_pointer->Tens_of_hours--;}
      }else if(RTC_buffer_pointer->discharge_selection==2)
      {
        if(RTC_buffer_pointer->Units_of_hours>0)
        {RTC_buffer_pointer->Units_of_hours--;}
      }else if(RTC_buffer_pointer->discharge_selection==3)
      {
        if(RTC_buffer_pointer->Tens_of_minutes>0)
        {RTC_buffer_pointer->Tens_of_minutes--;}
      }else if(RTC_buffer_pointer->discharge_selection==4)
      {
        if(RTC_buffer_pointer->Units_of_minutes>0)
        {RTC_buffer_pointer->Units_of_minutes--;}
      }  
    }
    if(IS_THE_BUTTON_3_PRESSED)//for switch a discharge to another discharge
    {
      while(IS_THE_BUTTON_3_PRESSED){;}
      delay_without_interruption(250);
      
      RTC_buffer_pointer->discharge_selection++;
      if(RTC_buffer_pointer->discharge_selection>4)
      {RTC_buffer_pointer->discharge_selection=1;}
    }
  }
}

//##---------------------------------------------------------------------------##//menu_selection_mode
void menu_selection_mode(RTC_structure * RTC_buffer_pointer)
{
  unsigned int press_duration=0;
  
  while((IS_THE_BUTTON_4_PRESSED)&&(press_duration<TWO_SECOND_DELAY))
  {press_duration++;}
  
  if(press_duration>=TWO_SECOND_DELAY)//reset the alarm
  {
    numbers_selection_signal();
    numbers_selection_signal();
    numbers_selection_signal();
    numbers_selection_signal();
    alarm_on_off=ALARM_OFF;
    RTC->CRH&=~RTC_CRH_ALRIE;//Disable the alarm interruption
    RTC->CRL&=~RTC_CRL_ALRF;//Reset the alarm interruption flag
  }else if((press_duration<TWO_SECOND_DELAY)&&(press_duration>HALF_SECOND_DELAY))//set the time
  {
    numbers_selection_signal();
    numbers_selection_signal();
    numbers_selection(RTC_buffer_pointer);
    RTC_Set_Time(RTC_buffer_pointer);
    
    numbers_selection_signal();    
  }else if(press_duration<=HALF_SECOND_DELAY)//set the alarm
  {
    numbers_selection_signal();
    //Alarm->Time
    RTC_buffer_pointer->Tens_of_hours=RTC_buffer_pointer->Alarm_Tens_of_hours;
    RTC_buffer_pointer->Units_of_hours=RTC_buffer_pointer->Alarm_Units_of_hours;
    RTC_buffer_pointer->Tens_of_minutes=RTC_buffer_pointer->Alarm_Tens_of_minutes;
    RTC_buffer_pointer->Units_of_minutes=RTC_buffer_pointer->Alarm_Units_of_minutes;
    
    numbers_selection(RTC_buffer_pointer);//Get new alarm values
    
    //Time->Alarm
    RTC_buffer_pointer->Alarm_Tens_of_hours=RTC_buffer_pointer->Tens_of_hours;
    RTC_buffer_pointer->Alarm_Units_of_hours=RTC_buffer_pointer->Units_of_hours;
    RTC_buffer_pointer->Alarm_Tens_of_minutes=RTC_buffer_pointer->Tens_of_minutes;
    RTC_buffer_pointer->Alarm_Units_of_minutes=RTC_buffer_pointer->Units_of_minutes;
    
    RTC_Set_Alarm(RTC_buffer_pointer);
    
    numbers_selection_signal();    
  }
  
  while(IS_THE_BUTTON_4_PRESSED){;}
  delay_without_interruption(250);
}