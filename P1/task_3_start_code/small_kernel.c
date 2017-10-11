
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
	printstr("It's kernel\n"); 
	return(0);
}
