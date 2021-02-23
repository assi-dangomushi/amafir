# amafir
fft convolution program  
input: stdin 32bit int 2ch  
output: stdout 32bit int 8ch  

## Usage
amafir2 coefDirectory  
coef-files : LF.txt, RF.txt, LB,txt, RB.txt, LS.txt, RS,txt, CF.txt  
LF,LB,LS:Left channel  
RF,RB,RS:Right channel  
CF:(L+R)/2  

## Notice
ChannelMapping is Changed(2021/02/23)

0: LF  
1: RF  
2: LB  
3: RB  
4: CF  
5: none  
6: LS  
7: RS  

Use with hdmi_play2.bin or ALSA.
Not use with hdmi_play.bin !
