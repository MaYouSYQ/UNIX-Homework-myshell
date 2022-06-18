#include<stdio.h>
int main()
{
	char c;
	int a = 0;
	while((c=getchar()) != EOF)
	{
		if(c == '\n') putchar(c);
		else putchar(c+1);
	}
	return 0;
}
