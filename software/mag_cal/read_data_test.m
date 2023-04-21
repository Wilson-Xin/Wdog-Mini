clear all;
close all;
clc
figure

global temp_buff;           
global uart_flag;                        
global uart_num;

 temp_buff=zeros(1,10);
 uart_flag=0;                        
 uart_num = 0; 

mx=zeros(1,16768);
my=zeros(1,16768);
mz=zeros(1,16768);
mi=0;

data_hex=textread('ist_maj1.txt','%s');
data_dec=hex2dec(data_hex);
for i=1:167680
receive_uart(data_dec(i));
 if uart_flag==1 
    mi=mi+1;
    %mx(mi) = temp_buff(4) * 256 + temp_buff(3);
    mx(mi) = typecast(uint8([temp_buff(3),temp_buff(4)]),'int16');
    %my(mi) = temp_buff(6) * 256 + temp_buff(5);
    my(mi) = typecast(uint8([temp_buff(5),temp_buff(6)]),'int16');
    %mz(mi) = temp_buff(8) * 256 + temp_buff(7);
    mz(mi) = typecast(uint8([temp_buff(7),temp_buff(8)]),'int16');

    uart_flag=0;
 end
 

end
scatter3(mx,my,mz)
%scatter3(mx,my,0*ones(1,length(mz)))
%plot(mx,mz,'-b.','MarkerSize',10);



function receive_uart(data)
    global uart_flag;
    global uart_num;
    global temp_buff;  
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