/*

// 本程序基于江协科技51单片机入门教程的开源代码，对江协科技在开源社区的无私贡献表示感谢
// 课程链接：https://www.bilibili.com/video/BV1Mb411e7re?spm_id_from=333.788.videopod.episodes&vd_source=4f296841ea6b433f1b35b1d40226e77d&p=24

*/

/*

// 此程序为湖南科技大学2022级电气（卓越）班单片机课程设计第12题多功能闹钟源代码
// (1) 时钟功能：具有24小时或12小时的计时方式，显示时、分、秒。
// (2) 具有快速校准时、分、秒的功能。
// (3) 能设定起闹时刻，响闹时间为1分钟，超过1分钟自动停；具有人工止闹功能；止闹后不再重新操作，将不再发生起闹。
// 扩展功能：能同时设置3个闹钟。

*/

//=============================================================================================
//=============================================================================================
//=============================================================================================
//
// Date			Author          Notes
// 20/11/2024	YHY             Initial release
//
//=============================================================================================
//=============================================================================================
//=============================================================================================


#include <REGX52.H>
#include "LCD1602.h"
#include "DS1302.h"
#include "Key.h"
#include "Timer0.h"
#include "Buzzer.h"
#include "MatrixKey.h"
#include "math.h"

//=============================================================================================
// 记录程序运行的总毫秒数，这里没有用到
volatile unsigned int millis;

//=============================================================================================
// 变量声明
unsigned char timeMode, clockMode;
unsigned char keyNum, matrixKeyNum;
unsigned char flashFlag;
char timeSetSelect, clockSetSelect;
char clockCnt, clockI, clockRing, lastMinute;
int inputDigit;
char clock[3][3] = {{7, 0, 0}, {13, 30, 0}, {22, 0, 0}};

//=============================================================================================
// 显示时间
void TimeShow(void) {
	DS1302_ReadTime();// 读取时间
	LCD_ShowNum(1, 1, DS1302_Time[0], 2);// 显示年
	LCD_ShowNum(1, 4, DS1302_Time[1], 2);// 显示月
	LCD_ShowNum(1, 7, DS1302_Time[2], 2);// 显示日
	LCD_ShowNum(2, 1, DS1302_Time[3], 2);// 显示时
	LCD_ShowNum(2, 4, DS1302_Time[4], 2);// 显示分
	LCD_ShowNum(2, 7, DS1302_Time[5], 2);// 显示秒
}

//=============================================================================================
// 校准时间
void TimeSet(void) {
    
    // 矩阵按键1 ~ 10输入数据给DS1302_Time数组
    // 矩阵按键11让输入数据位左移一位
    // 矩阵按键11让输入数据位右移一位
    if (matrixKeyNum > 0 && matrixKeyNum <= 10) {
        inputDigit += matrixKeyNum % 10;
        DS1302_Time[timeSetSelect] = inputDigit;
        if(inputDigit >= 10) inputDigit = 0;
        inputDigit *= 10;
    } else if (matrixKeyNum == 11) {   
        targetNumChange(&timeSetSelect, -1, 0, 5);
	} else if (matrixKeyNum == 12) {
        targetNumChange(&timeSetSelect, 1, 0, 5);
    }
    
    // 限制数据范围满足年月日，时分秒
    targetNumChange(&DS1302_Time[0], 0, 0, 99);
    targetNumChange(&DS1302_Time[1], 0, 1, 12);
    if (DS1302_Time[1] == 1 || DS1302_Time[1] == 3 || DS1302_Time[1] == 5 || 
        DS1302_Time[1] == 7 || DS1302_Time[1] == 8 || DS1302_Time[1] == 10 || 
        DS1302_Time[1] == 12) {
        targetNumChange(&DS1302_Time[2], 0, 1, 31);
    }
    else if(DS1302_Time[1] == 4 || DS1302_Time[1] == 6 || DS1302_Time[1] == 9 || DS1302_Time[1] == 11) {
        targetNumChange(&DS1302_Time[2], 0, 1, 30);
    }
    else if(DS1302_Time[1] == 2) {
        if(DS1302_Time[0] % 4) {
            targetNumChange(&DS1302_Time[2], 0, 1, 28);
        }
        else {
            targetNumChange(&DS1302_Time[2], 0, 1, 29);
        }
    }
    targetNumChange(&DS1302_Time[3], 0, 0, 23);
    targetNumChange(&DS1302_Time[4], 0, 0, 59);
    targetNumChange(&DS1302_Time[5], 0, 0, 59);
    
	// 更新显示
	if(timeSetSelect == 0 && flashFlag == 1){LCD_ShowString(1, 1, "  ");}
	else {LCD_ShowNum(1, 1, DS1302_Time[0], 2);}
	if(timeSetSelect==1 && flashFlag==1){LCD_ShowString(1, 4, "  ");}
	else {LCD_ShowNum(1, 4, DS1302_Time[1], 2);}
	if(timeSetSelect==2 && flashFlag==1){LCD_ShowString(1, 7, "  ");}
	else {LCD_ShowNum(1, 7, DS1302_Time[2], 2);}
	if(timeSetSelect==3 && flashFlag==1){LCD_ShowString(2, 1, "  ");}
	else {LCD_ShowNum(2, 1, DS1302_Time[3], 2);}
	if(timeSetSelect==4 && flashFlag==1){LCD_ShowString(2, 4, "  ");}
	else {LCD_ShowNum(2, 4, DS1302_Time[4], 2);}
	if(timeSetSelect==5 && flashFlag==1){LCD_ShowString(2, 7, "  ");}
	else {LCD_ShowNum(2, 7, DS1302_Time[5], 2);}
}

//=============================================================================================
// 显示闹钟，将第cnt + 1个响铃时间输出到LCD1602上
void clockShow(char cnt) {
    LCD_ShowString(1, 10, "Clock :");
    LCD_ShowNum(1, 15, cnt + 1, 1);
	LCD_ShowNum(2, 10, clock[cnt][0], 2);// 显示时
	LCD_ShowNum(2, 13, clock[cnt][1], 2);// 显示分
    
}

//=============================================================================================
// 设置闹钟，设置第cnt + 1个闹钟的响铃时间
void clockSet(char cnt) {
    LCD_ShowNum(1, 15, cnt + 1, 1);
    
    // 矩阵按键1 ~ 10输入数据给clock数组
    // 矩阵按键11让输入数据位左移一位
    // 矩阵按键12让输入数据位右移一位
    if (matrixKeyNum > 0 && matrixKeyNum <= 10) {
        inputDigit += matrixKeyNum % 10;
        clock[cnt][clockSetSelect] = inputDigit;
        if(inputDigit >= 10) inputDigit = 0;
        inputDigit *= 10;
    } else if (matrixKeyNum == 11) {   
        targetNumChange(&clockSetSelect, -1, 0, 1);
	} else if (matrixKeyNum == 12) {
        targetNumChange(&clockSetSelect, 1, 0, 1);
    }    
    
    targetNumChange(&clock[cnt][0], 0, 0, 23);
    targetNumChange(&clock[cnt][1], 0, 0, 59);
   
	// 更新显示
	if (clockSetSelect == 0 && flashFlag == 1) {
        LCD_ShowString(2, 10, "  ");
    } else {
        LCD_ShowNum(2, 10, clock[cnt][0], 2);
    }
    
	if (clockSetSelect == 1 && flashFlag == 1) {
        LCD_ShowString(2, 13, "  ");
    } else {
        LCD_ShowNum(2, 13, clock[cnt][1], 2);
    }
}

//=============================================================================================
// 主函数
void main(void)
{
    // 初始化输出模块LCD1602
	LCD_Init();
    
    // 初始化时钟模块DS1302
	DS1302_Init();
    
    // 初始化定时计数器0
	Timer0Init();
    
	// 将默认时间从RAM输出到DS1302模块
	DS1302_SetTime(); 
    
    // 更新上一次检测响铃的时间
    lastMinute = DS1302_Time[4] - 1;
    
    // 静态字符初始化显示
	LCD_ShowString(1, 1, "  -  -  "); 
	LCD_ShowString(2, 1, "  :  :  ");
	LCD_ShowString(2, 12, ":");
    
    // 主循环
	while(1)
	{
        // 按键输入
        matrixKeyNum = MatrixKey();
        keyNum = Key();
        
        
        // 如果独立按键输入，显示当前时间或某个响铃时间，并如果对应闹钟正在响铃，进行止闹
        if (keyNum) {
            switch(keyNum) {
                case 1: TimeShow(); break;
                case 2: clockCnt = 0; break;
                case 3: clockCnt = 1; break;
                case 4: clockCnt = 2; break;
            }    
            clockShow(clockCnt);
            if(clockCnt == clockRing) clockRing = 3;
        }
        
        // 如果矩阵按键13按下，进行时间校准，校准完毕后再按下则将校准时间输出到DS1302时钟模块
        // 如果矩阵按键14按下，进行当前所显示闹钟的响铃时间调整
		if (matrixKeyNum == 13) 
		{
            if (timeMode == 0) {
                timeSetSelect = 0;
            } else if (timeMode == 1) {
                DS1302_SetTime();
            }
            timeMode ^= 1;
		} else if (matrixKeyNum == 14) {
            clockSetSelect = 0;
            clockMode ^= 1;
		}
        
        
        // 如果当前时间模式为0，那么显示时间
        // 如果当前时间模式为1，那么校准时间
        if(timeMode == 0) {
            TimeShow();
        } else if (timeMode == 1) {
            TimeSet();
        }
        
        
        // 如果当前闹钟模式为0，那么显示对应闹钟
        // 如果当前闹钟模式为1，那么设置对应闹钟
        if(clockMode == 0) {
            clockShow(clockCnt);
        } else if (clockMode == 1) {
            clockSet(clockCnt);
        }
        
        
        // 固定一分钟检测是否有闹钟满足响铃条件
        // 如果检测到某个闹钟满足响铃条件，记录下来并响铃
        if(DS1302_Time[4] != lastMinute) {
            lastMinute = DS1302_Time[4];
            for(clockI = 0; clockI < 3; ++clockI) {
                if(DS1302_Time[3] == clock[clockI][0] && 
                   DS1302_Time[4] == clock[clockI][1] && 
                   DS1302_Time[5] >= clock[clockI][2]) {     
                    clockRing = clockI; break;
                } else {
                    clockRing = 3;
                }
            }   
        }
        
        // clockRing = 0 ~ 2 表示闹钟0 ~ 2正在响铃
        // clockRing = 3 表示无闹钟起闹
        if(clockRing != 3) Buzzer_Time(1);
        
  	}
}

//=============================================================================================
// 定时器0中断，每500ms将闪烁标志位取反
void Timer0_Routine() interrupt 1
{
	static unsigned int T0Count;
	TL0 = 0x18;		// 设置定时初值
	TH0 = 0xFC;		// 设置定时初值
    ++millis;
	T0Count++;
	if(T0Count >= 500)// 每500ms进入一次
	{
		T0Count = 0;
		flashFlag ^= 1; // 闪烁标志位取反
	}
}
