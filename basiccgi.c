#include <stdio.h>

int main(int argc,char **argv,char**envp)
{
    argc=argc; argv=argv; envp=envp;
    printf("Content-type: text/html\n\n");

    printf("<html><title>Hello</title><body>\n");
    printf("Goodbye Cruel World\n");
    printf("<pre>\n");
    printf("envs\n");
    while(*envp!=NULL){
	printf("    %s\n",*envp++);
    }
    printf("End of envs\n");
    printf("</pre>\n");

    printf("</body></html>\n");
    fflush(stdout);
    return 0;
}
