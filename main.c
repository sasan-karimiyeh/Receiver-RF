#include <TINY24.h>
#include <delay.h>
#include <stdio.h>
///////////////////////////
#define led PORTA.7
#define up_down PORTA.5
#define com PORTA.6
#define up PINA.0
#define down PINB.0
#define learn PINA.2
#define rev PINA.1
#define data PINB.2
////////////////////////////
bit dir=0, db=0, man=0,mf=0;
int t0ovf,t0f,t0time,b=0,condition=0,k,s;
int address[24];     //buffer
int cof[10]={512,256,128,64,32,16,8,4,2,1};
eeprom int d;
eeprom int address_rec[62];
////////////////////////////
void learn_com(void){
/*
 condition=0 ==> normal operation led normal
 condition=1 ==> learning mode led 1Hz 
 condition=2 ==> learning complete led normal
 condition=3 ==> one memory delete mode led 0.5Hz
*/
TIMSK0=0x01;
TCCR0A=0x00;
TCCR0B=0x04;
TCNT0=0;
t0ovf=0;
t0f=0;
t0time=0;
condition=1;
mf=1;
}
////////////////////////////
void t0ovf_isr(void){
t0ovf++;
t0time++;
 if(t0ovf>60 && condition==1){
 t0ovf=0;
 led=~led;
 } 
 
 if(t0ovf>30 && condition==3){
 t0ovf=0;
 led=~led;
 }
}
////////////////////////////
void command(int a){
/*
a==1 ==> up
a==2 ==> down
a==3 ==> stop
*/
if(a!=s){
 switch (a){
 case 1: //up       
 man=0;
 com=0;
 delay_ms(200);
 com=1;
 if(dir==0){up_down=0;}
 if(dir==1){up_down=1;}
 break;
 case 2: //down 
 man=0;
 com=0;
 delay_ms(200);
 com=1;
 if(dir==0){up_down=1;}
 if(dir==1){up_down=0;}
 break;
 case 3: //stop
 com=0;
 break;
 default:
 break;
 }
 }
 s=a;
}
////////////////////////
void main (void){
DDRA=0xf0;
DDRB=0x00;
PORTA=0x0f;
PORTB=0xff;
MCUCR=0x01;
GIFR=0x40;
GIMSK=0x40;
TIMSK1=0x01;
TCCR1A=0x00;
TCCR1B=0x03;
led=1;
if(d<0){d=0;}
#asm("sei")
 while(1){ 
 if(learn==0 && db==0){db=1;learn_com();}
 if(rev==0 && db==0){db=1;dir=~dir;} 
 /* 
 man==> manual debounce
 dir ==> direction
 db ==> general debounce 
 mf ==> memory delete flag
 */
  while(up==0){ 
   if(man==0){
   com=1;
   if(dir==0){up_down=0;}
   if(dir==1){up_down=1;} 
   }
   man=1;
  }
  
  while(down==0){
   if(man==0){
   com=1;
   if(dir==0){up_down=1;}
   if(dir==1){up_down=0;} 
   }
   man=1;
  }
 if(man==1){man=0;com=0;}
 if(learn && rev && db){db=0;} 
 if(t0f>0){t0f--;t0ovf_isr();}   
  if((condition==1 && t0time>900 && mf==0) || (condition==3 && t0time>1800 && mf==0)|| condition==2){
  TCCR0A=0;
  TCCR0B=0;
  TCNT0=0; 
  led=0;
  delay_ms(500);
  led=1;
  condition=0;
  }
   
  if(k==2){
  k=0;                      //commands
  if(address[23]==1){command(1);}
  if(address[22]==1){command(2);}
  if(address[21]==1){command(3);}
  }
  if(learn==1){mf=0;}  
  if(condition==1 && t0time>900 && mf==1){
  condition=3;
  }
  
  if(condition==3 && t0time>1800 && mf==1){//all memory delete mode
   for(k=0;k<d;k++){
   address_rec[0+(k*2)]=0;
   address_rec[1+(k*2)]=0; 
   d=0;
   led=0;
   delay_ms(2000);
   led=1;
   TCCR0A=0;
   TCCR0B=0;
   TCNT0=0;      
   condition=0;
   }
   k=0;
  }
 }
}
////////////////////////////
 interrupt[TIM0_OVF] void counter (void){
 t0f++;
 }
////////////////////////////
interrupt[EXT_INT0] void reciever (void){
int time,i,j;
static int a;
int address_buff[2]; //buffer
 time=TCNT1*8;
 TCNT1=0;
  if(a==0 && time<12350 && time>8000 && data){  //premable
  a++; 
   for(i=0;i<=24;i++){
   address[i]=0;
   }        
  goto loop1;
  }

  if(a==1 && time<1200 && time>750 && data==0){
  a=2;
  goto loop1;
  }

  if(a==2&& time<600 && time>250 && data){
  address[b]=1;
  b++;
  a=1;
   if(b==24){
   goto loop2;
   }
  goto loop1;
  }  
  
  if(a==1&& time>100 && time<550 && data==0){
  a=3;
  goto loop1;
  }
  
  
  if(a==3 && time<1350 && time>800 && data){ 
  address[b]=0;
  b++;
  a=1;
   if(b==24){
   goto loop2;
   }
  goto loop1;
  }  
  loop2:
  if(b==24){
   for(i=0;i<2;i++){
   address_buff[i]=0;
   }
   
   //integer combine
   for(i=0;i<10;i++){
   address_buff[0]=address_buff[0]+(address[i]*cof[i]);
   address_buff[1]=address_buff[1]+(address[i+10]*cof[i]);
   }    
   
   if(condition==0){         //non learning 
    for(i=0;i<=d;i++){
    k=0;
     if(address_buff[0]==address_rec[i*2]){k++;}
     if(address_buff[1]==address_rec[1+(i*2)]){k++;}    
     if(k==2){goto loop3;}   //correct
    }
    loop3:
   }
   
   if(condition==1 && d<2){                    //learning mode
   condition=2;
    for(i=0;i<=d;i++){
    k=7;
     for(j=0;j<2;j++){
     if(address_buff[j]==address_rec[j+(i*2)]){k++;}
     }
     if(k==9){goto loop6;}  //redefine
    }

    for(i=0;i<2;i++){       //define new one
    address_rec[i+(d*2)]=address_buff[i];
    }
    delay_ms(150);
    d++;
    loop6:
   } 
   
   if(condition==3){   //one memory delete mode
    for(i=0;i<=d;i++){
    k=12;
     for(j=0;j<2;j++){
     if(address_buff[j]==address_rec[j+(i*2)]){k++;}
     }
     
     if(k==14){  //one memory delete
     address_rec[0+(i*2)]=0;
     address_rec[j+(i*2)]=0;
     led=0;
     delay_ms(2000);
     led=1;
     TCCR0A=0;
     TCCR0B=0;
     TCNT0=0;      
     condition=0;
     }  
    }
   }
   delay_ms(20);
  }      
  a=0;
  b=0; 
  loop1:    
 }
