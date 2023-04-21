clear all;
close all;
clc
f1=figure;

get_piont=20000;%获取30000个点
% global temp_buff;           
% global uart_flag;                        
% global uart_num;

 temp_buff=zeros(1,10);
 uart_flag=0;                        
 uart_num = 0; 

%读3万个点
mx=zeros(1,get_piont);
my=zeros(1,get_piont);
mz=zeros(1,get_piont);
mi=0;

try
    s=serialport("COM5",115200);%配置串口
catch
    error('cant serial');
end

while mi < get_piont

  get_char=read(s,1,"uint8");%读取串口
  [uart_flag,uart_num,temp_buff]=receive_uart(get_char,uart_flag,uart_num,temp_buff);
   if uart_flag==1 
      mi=mi+1;
      %mx(mi) = temp_buff(4) * 256 + temp_buff(3);
      mx(mi) = typecast(uint8([temp_buff(3),temp_buff(4)]),'int16');
      %my(mi) = temp_buff(6) * 256 + temp_buff(5);
      my(mi) = typecast(uint8([temp_buff(5),temp_buff(6)]),'int16');
      %mz(mi) = temp_buff(8) * 256 + temp_buff(7);
      mz(mi) = typecast(uint8([temp_buff(7),temp_buff(8)]),'int16');
  
      uart_flag=0;
      plot3(mx(mi),my(mi),mz(mi),'-b.','MarkerSize',5) %实时显示采集图像-慢但有用
      grid on
      hold on
   end
end 
clear device
close(f1);%由于太卡，关闭重建

f2=figure;
plot3(mx,my,mz,'-b.','MarkerSize',5)%原始椭球


%以下做校准-最小二乘法
%参数求解
num_points = length(mx);
%一次项统计平均
x_avr = sum(mx)/num_points;
y_avr = sum(my)/num_points;
z_avr = sum(mz)/num_points;
%二次项统计平均
xx_avr = sum(mx.*mx)/num_points;
yy_avr = sum(my.*my)/num_points;
zz_avr = sum(mz.*mz)/num_points;
xy_avr = sum(mx.*my)/num_points;
xz_avr = sum(mx.*mz)/num_points;
yz_avr = sum(my.*mz)/num_points;
%三次项统计平均
xxx_avr = sum(mx.*mx.*mx)/num_points;
xxy_avr = sum(mx.*mx.*my)/num_points;
xxz_avr = sum(mx.*mx.*mz)/num_points;
xyy_avr = sum(mx.*my.*my)/num_points;
xzz_avr = sum(mx.*mz.*mz)/num_points;
yyy_avr = sum(my.*my.*my)/num_points;
yyz_avr = sum(my.*my.*mz)/num_points;
yzz_avr = sum(my.*mz.*mz)/num_points;
zzz_avr = sum(mz.*mz.*mz)/num_points;
%四次项统计平均
yyyy_avr = sum(my.*my.*my.*my)/num_points;
zzzz_avr = sum(mz.*mz.*mz.*mz)/num_points;
xxyy_avr = sum(mx.*mx.*my.*my)/num_points;
xxzz_avr = sum(mx.*mx.*mz.*mz)/num_points;
yyzz_avr = sum(my.*my.*mz.*mz)/num_points;


%计算求解线性方程的系数矩阵
A0 = [yyyy_avr yyzz_avr xyy_avr yyy_avr yyz_avr yy_avr;
     yyzz_avr zzzz_avr xzz_avr yzz_avr zzz_avr zz_avr;
     xyy_avr  xzz_avr  xx_avr  xy_avr  xz_avr  x_avr;
     yyy_avr  yzz_avr  xy_avr  yy_avr  yz_avr  y_avr;
     yyz_avr  zzz_avr  xz_avr  yz_avr  zz_avr  z_avr;
     yy_avr   zz_avr   x_avr   y_avr   z_avr   1;];
%计算非齐次项
b = [-xxyy_avr;
     -xxzz_avr;
     -xxx_avr;
     -xxy_avr;
     -xxz_avr;
     -xx_avr];

resoult = inv(A0)*b;

x00 = -resoult(3)/2;                  %拟合出的x坐标
y00 = -resoult(4)/(2*resoult(1));     %拟合出的y坐标
z00 = -resoult(5)/(2*resoult(2));     %拟合出的z坐标

AA = sqrt(x00*x00 + resoult(1)*y00*y00 + resoult(2)*z00*z00 - resoult(6));   % 拟合出的x方向上的轴半径
BB = AA/sqrt(resoult(1));                                                    % 拟合出的y方向上的轴半径
CC = AA/sqrt(resoult(2));                                                    % 拟合出的z方向上的轴半径

%校准-去零飘
mx=mx-x00;
my=my-y00;
mz=mz-z00;
%椭球转换为球
my=my*(AA/BB);
mz=mz*(AA/CC);
f3=figure;
plot3(mx,my,mz,'-b.','MarkerSize',5)%校准后的图片


%串口获取有效数据函数
function [uart_flag,uart_num,temp_buff]=receive_uart(data,uart_flag,uart_num,temp_buff)
%     global uart_flag;
%     global uart_num;
%     global temp_buff;  
    LINE_LEN=10;
    uart_num=uart_num+1;
    temp_buff(uart_num) = data;
    
    if(1 == uart_num)  
  
        
        if 0Xfe ~= temp_buff(1)   
            uart_num = 0;  
            uart_flag = 3;  
        end  
    end 

    if(2 == uart_num)  
  
        
        if 0Xef ~= temp_buff(2)   
            uart_num = 0;  
            uart_flag = 3;  
        end  
    end  

    if(LINE_LEN-1 == uart_num)  
  
        
        if 0Xde ~= temp_buff(LINE_LEN-1)   
            uart_num = 0;  
            uart_flag = 3;  
        end  
    end  

     if LINE_LEN == uart_num   
      
            uart_flag = 2;  
 
        if 0Xed==temp_buff(LINE_LEN)  
         
            uart_flag = 1; 
            
        else    
         
            uart_flag = 3;  
        end  

           uart_num = 0;  
     end     

end