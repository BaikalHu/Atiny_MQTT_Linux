#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "agent_tiny_demo.h"

int main()
{
    pthread_t tidp;

    if ((pthread_create(&tidp, NULL, agent_tiny_entry, NULL)) == -1)
    {
        printf("create error!\n");
        return 1;
    }
    while(1);
    printf("end\n");

    
    return 0;
}
