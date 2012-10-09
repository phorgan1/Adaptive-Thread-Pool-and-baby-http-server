#include <stdio.h>

int main(int argc,char **argv,char**envp)
{
    printf("Content-type: text/html\n\n");
    printf("<html><title>Hello</title><body>\n");
    printf("Goodbye Cruel World\n");
    printf("<pre>\n");
    while(*envp!=NULL){
	printf("%s\n",*envp++);
    }
    printf("</pre>\n");

    printf("</body></html>");
    return 1;
}
