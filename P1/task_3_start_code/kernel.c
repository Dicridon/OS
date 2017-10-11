
#define PORT3f8 0xbfe48000
#define PORT3fd 0xbfe48006

void delay(unsigned long n)
{
    while (n > 0) {
        n--;
    }
}

void printstr(char *s)
{
   unsigned long port = PORT3f8; 
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
     	*(unsigned char*)port = *s;
    	s++;
   }
}
int main(){
	printstr("It's kernel!\r\n");
    printstr("It's kernel!\r\n");
	return(0);
}

/*extend space here*/
void printstr0(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr1(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr2(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr3(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr4(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr5(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr6(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr7(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr8(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr9(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr10(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr11(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr12(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr13(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr14(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr15(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr16(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr17(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr18(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr19(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr20(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr21(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr22(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr23(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr24(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr25(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr26(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr27(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr28(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
void printstr29(char *s){
   unsigned long port = PORT3f8;
   delay(10000);
   while (*s) {
     /* while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0);  */
        *(unsigned char*)port = *s;
        s++;
   }
}
