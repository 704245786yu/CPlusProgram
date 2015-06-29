/*通过两层指针参数分配内存
 * para_allocator.c
 * */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "para_allocator.h"

void alloc_struct(struct_t **pp)
{
	struct_t *p = malloc(sizeof(struct_t));
	if(p==NULL){
		printf("out of memory\n");
		exit(1);
	}
	p->number = 3;
	p->msg = malloc(20);
	strcpy(p->msg,"Hello World!");
	*pp = p;
}

void free_struct(struct_t *p)
{
	free(p->msg);
	free(p);
}
