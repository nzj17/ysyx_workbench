#include </home/nzj17/ysyx-workbench/am-kernels/tests/am-tests/include/amtest.h>
#define W 256
#define H 256
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <am.h>
#include </home/nzj17/dbg.h>
#include <klib-macros.h>
#include </home/nzj17/dbg.h>
#define ff 0b00000000000000000000000000000100
uint32_t  word_address=0;
uint32_t  three_zero_byte_choice=0;
uint32_t  RAM_byte_address=0;
uint32_t temp=0;
uint32_t PC=0;
uint32_t Big_Endian_instruction=0;
uint32_t  Big_Endian_RAM_Data;
uint32_t GPR[16]={0};	
uint32_t  ROM[142280]={0};
uint32_t  RAM[142280]={0};
uint32_t addr_instruction;
uint32_t life;
static uint32_t VGA[256][256];
void  printx(char * name,uint32_t number) {
	printf("%s\t%#x \n",name,number);
}
uint32_t getbits(uint32_t number , unsigned char begin ,unsigned char end) {
uint32_t lu=0b00000000000000000000000000000001;
uint32_t mask=((lu<<(end-begin+lu))-lu);
		return mask&(number>>begin);
}

uint32_t inst_cycle(uint32_t addr_instruction) {
   
   Big_Endian_instruction=ROM[addr_instruction];
// printx("Big_Endian_instruction",Big_Endian_instruction);
	uint32_t fourteen_twelve=getbits(Big_Endian_instruction,12,14);
//	 printx("fourteen_twelve",fourteen_twelve);
    uint32_t  six_zero=getbits(Big_Endian_instruction,0,6);
 //   	 printx("six_zero",six_zero);
    uint32_t  eleven_seven=getbits(Big_Endian_instruction,7,11);
 //   	 printx("eleven_seven",eleven_seven);
    uint32_t  nineteen_fifteen=getbits(Big_Endian_instruction,15,19);
//    	 printx( "nineteen_fifteen",nineteen_fifteen);
    uint32_t thirty_one_twenty=getbits(Big_Endian_instruction,20,31);
//    	 printx("thirty_one_twenty",thirty_one_twenty);
    uint32_t thirty_one_twelve=getbits(Big_Endian_instruction,12,31);
//   	 printx("thirty_one_twelve",thirty_one_twelve);
    uint32_t thirty_one_twenty_five=getbits(Big_Endian_instruction,25,31);
//    	 printx("thirty_one_twenty_five",thirty_one_twenty_five);
  uint32_t or_thirty_one_twenty=getbits(thirty_one_twenty,11,11)?0b11111111111111111111000000000000:0;
    uint32_t signed_thirty_one_twenty=thirty_one_twenty | or_thirty_one_twenty;  
  uint32_t integrated_imm =(thirty_one_twenty_five<<5)|(eleven_seven);
 uint32_t or_integrated_imm=getbits(integrated_imm,11,11)?0b11111111111111111111000000000000:0;
    uint32_t signed_integrated_imm=or_integrated_imm | integrated_imm;
    uint32_t twenty_four_twenty=getbits(Big_Endian_instruction,20,24);
     
   // uint32_t fir_zero=getbits(Big_Endian_instruction,0,1);
   // uint32_t fifth_second=getbits(Big_Endian_instruction,2,5);
   // uint32_t third_zero=getbits(Big_Endian_instruction,0,3);
   
    switch(six_zero) {
       	case 0b0010011: {
              GPR[eleven_seven]=GPR[nineteen_fifteen]+signed_thirty_one_twenty;
              PC=PC+ff;
			break;
		}
    	case 0b1100111: { 
    	 //   GPR[eleven_seven]=PC+ff;
    	      uint32_t midddle=PC;
          PC=(GPR[nineteen_fifteen]+signed_thirty_one_twenty) & 0xFFFFFFFFE;
          GPR[eleven_seven]=midddle+ff;
		   /*JALR */
			break;
		}
    	case 0b0110011: {
    		GPR[eleven_seven]=GPR[nineteen_fifteen]+GPR[twenty_four_twenty];
            PC=PC+ff;
			break;
					}
		case 0b0110111: {
            GPR[eleven_seven]=(thirty_one_twelve<<12);
       //      printx("thirty_one_twenty",thirty_one_twelve);
       //      printx("thirty_one_twenty<<12",thirty_one_twelve<<12);
       //      printf("%#x   %#x  \n ", GPR[eleven_seven],eleven_seven);
			PC=PC+ff;
			break;	
	    }
        case 0b0000011:{
            switch(fourteen_twelve){	
//LW
                case 0b010 : { RAM_byte_address=GPR[nineteen_fifteen]+signed_thirty_one_twenty;
                  word_address=RAM_byte_address>>2;
                   Big_Endian_RAM_Data=RAM[word_address];
                 GPR[eleven_seven]= Big_Endian_RAM_Data;
				  PC=PC+ff;
                    break;
                }
                case 0b100 : {
//LBU
                RAM_byte_address=GPR[nineteen_fifteen]+signed_thirty_one_twenty;
                  word_address=RAM_byte_address>>2;
                 three_zero_byte_choice =RAM_byte_address&0b00000000000000000000000000000011;
                   GPR[eleven_seven]=(RAM[word_address]>>(three_zero_byte_choice*8))&0b00000000000000000000000011111111;
                    PC=PC+ff;
                    break;
                 }
            } 
                  break; 
         }
//JALR PC AND  GPR[nineteen_fifteen]1  AND  GPR[eleven_seven]1
// SIGNED  UNSIGNED 
// SW  SB  word_address=RAM_byte_address&0b11111111111111111111111111111100;
// LBU LW  word_address=RAM_byte_address&0b11111111111111111111111111111100;
// RAM 小段排序  GPR无排序大端排序  注意ss ll的大小端改变
// RAM 的数据不区分符号
// 区分末尾置0 与 除4
        case 0b0100011:{
             switch(fourteen_twelve){
//SW        
               case 0b010 : {
                Big_Endian_RAM_Data=GPR[twenty_four_twenty];
		RAM_byte_address=GPR[nineteen_fifteen]+signed_integrated_imm;
		if((RAM_byte_address<0x20040000)&&(RAM_byte_address>=0x20000000)) {
                	uint8_t x=(uint8_t)getbits(RAM_byte_address,2,9);
                    uint8_t y=(uint8_t)getbits(RAM_byte_address,10,17);
                    VGA[y][x]=((getbits(Big_Endian_RAM_Data,0,7))| ((getbits(Big_Endian_RAM_Data,8,15))<<8)| ((getbits(Big_Endian_RAM_Data,16,23))<<16)|(0x00<<24));      
                }
                else {
                  word_address=RAM_byte_address>>2;
                 RAM[word_address]=Big_Endian_RAM_Data;
                }
                 PC=PC+ff;
                    break;
            }
//SB 
                case 0b000 :{
                 RAM_byte_address=GPR[nineteen_fifteen]+signed_integrated_imm;                
                  word_address=RAM_byte_address>>2;
                 three_zero_byte_choice=RAM_byte_address&0b00000000000000000000000000000011;  
                  uint32_t trick=0xFF;
                  trick=~(trick<< (three_zero_byte_choice*8));
                temp=(GPR[twenty_four_twenty]&0x000000FF)<<(three_zero_byte_choice*8);
                
                if((RAM_byte_address<0x20040000)&&(RAM_byte_address>=0x20000000)) {
                	uint8_t x=(uint8_t)getbits(RAM_byte_address,2,9);
                    uint8_t y=(uint8_t)getbits(RAM_byte_address,10,17);
                    Big_Endian_RAM_Data=(RAM[word_address]& trick)| temp;  
                    VGA[y][x]=((getbits(Big_Endian_RAM_Data,0,7))| ((getbits(Big_Endian_RAM_Data,8,15))<<8)| ((getbits(Big_Endian_RAM_Data,16,23))<<16)|(0x00<<24));
                 }
		   else{
                RAM[word_address] = (RAM[word_address]& trick)| temp;
				}   
		         PC=PC+ff;
                 break;
                 }
            }
            break;
        }
    }
	return  Big_Endian_instruction;	
	}
        
int main(int argc,char *argv[]) {
      ioe_init();
// uint32_t test;
// int op=0;
	FILE* fp=fopen("/home/nzj17/vga.bin","rb");
	if(fp==NULL) {
		printf("died with failed file_open\n");
		return 1;
	}
 // test=fseek(fp,0,SEEK_END);
 //  check(~test,"fseek1");
 // long file_byte_number=ftell(fp);
 //check(~test,"ftell");
//  test=fseek(fp,0,SEEK_SET);
//check(~test,"fseek2");
 
    fread(ROM,4,142280,fp);
   //    	 check(test,"fread_ROM");
  fseek(fp,0,SEEK_SET);
 //  check(~test,"fseek3");
 
	fread(RAM,4,142280,fp);
	/*for(int k=0;k<=1000; k++) {
		printf("%#x          RAM[%d] \n",RAM[k],k);
	}  */
  //  check(test,"fread_RAM");
   printf("%#x \n",ROM[0]);
   printf("%#x \n",RAM[0]);
  fclose(fp);
  int count=0;
   for(int i=0;i<0x140000;i++,count++) {
   addr_instruction=PC>>2;
  life=inst_cycle(addr_instruction);
   GPR[0]=0;
   printx("big_zhiling  ",life);
   } 
io_write(AM_GPU_FBDRAW, 0, 0,(uint32_t *)VGA, W, H, false);
while(1) {}
return 0;  
/*error:
 printf("%#x \n",ROM[0]);
 return 1; */
}

