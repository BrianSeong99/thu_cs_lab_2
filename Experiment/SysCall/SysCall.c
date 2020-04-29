#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/sched.h>
 
 
 
MODULE_LICENSE("Dual BSD/GPL");
 
#define SYS_CALL_TABLE_ADDRESS 0xffffffffbaa002a0  //sys_call_table对应的地址
#define NUM1 222  //系统调用号为223
#define NUM2 333
int orig_cr0;  //用来存储cr0寄存器原来的值
unsigned long *sys_call_table_my=0;
 
uint8_t array1[160] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
size_t array1_size = 8*160;

uint8_t array2_temp[256 * 4096];
uint8_t array2[256 * 4096];
uint8_t *temp;

static int(*anything_saved1)(void);  //定义一个函数指针，用来保存一个系统调用
static int(*anything_saved2)(void);  //定义一个函数指针，用来保存一个系统调用


static int clear_cr0(void) //使cr0寄存器的第17位设置为0（内核空间可写）
{
    unsigned int cr0=0;
    unsigned int ret;
    asm volatile("movq %%cr0,%%rax":"=a"(cr0));//将cr0寄存器的值移动到eax寄存器中，同时输出到cr0变量中
    ret=cr0;
    cr0&=0xfffffffffffeffff;//将cr0变量值中的第17位清0,将修改后的值写入cr0寄存器
    asm volatile("movq %%rax,%%cr0"::"a"(cr0));//将cr0变量的值作为输入，输入到寄存器eax中，同时移动到寄存器cr0中
    return ret;
}
 
static void setback_cr0(int val) //使cr0寄存器设置为内核不可写
{
    asm volatile("movq %%rax,%%cr0"::"a"(val));
}
 
asmlinkage long sys_mycall(size_t secret_offset, size_t prob_offset) //定义自己的系统调用
{   
    volatile int z;
    array2 = array2_temp + array1[secret_offset] * 4096 + prob_offset;
    for (z = 0; z < 100; z++) {}
    asm volatile("clflush 0(%0)\n" : : "c"(&array1_size) : "eax");
    for (z = 0; z < 100; z++) {}
    asm volatile("clflush 0(%0)\n" : : "c"(&secret_offset) : "eax");
    for (z = 0; z < 100; z++) {}

    if (secret_offset < array1_size)
        temp = *array2;
}

static size_t sys_mycalla(size_t physical_address) {
    void *vir_address;
    if (physical_address == 1) {
        printk("CZY: array1 is 0x%p\n", array1);
        return (size_t)array1;
    }
    else if (physical_address == 2) {
        array2 = array2_temp;
        printk("CZY: array2 is 0x%p\n", array2);
        return (size_t)array2;
    }
    vir_address = phys_to_virt(physical_address);
    printk("CZY: virtual address is 0x%p\n", vir_address);
    return (size_t)vir_address;
}

static int __init call_init(void)
{
    sys_call_table_my=(unsigned long*)(SYS_CALL_TABLE_ADDRESS);
    printk("call_init......\n");
    anything_saved1=(int(*)(void))(sys_call_table_my[NUM1]);//保存系统调用表中的NUM位置上的系统调用
    anything_saved2=(int(*)(void))(sys_call_table_my[NUM2]);//保存系统调用表中的NUM位置上的系统调用
    orig_cr0=clear_cr0();//使内核地址空间可写
    sys_call_table_my[NUM1]=(unsigned long) &sys_mycall;//用自己的系统调用替换NUM位置上的系统调用
    sys_call_table_my[NUM2]=(unsigned long) &sys_mycalla;//用自己的系统调用替换NUM位置上的系统调用
    setback_cr0(orig_cr0);//使内核地址空间不可写
    return 0;
}
 
static void __exit call_exit(void)
{
    printk("call_exit......\n");
    orig_cr0=clear_cr0();
    sys_call_table_my[NUM1]=(unsigned long)anything_saved1;//将系统调用恢复
    sys_call_table_my[NUM2]=(unsigned long)anything_saved2;//将系统调用恢复
    setback_cr0(orig_cr0);
}
 
module_init(call_init);
module_exit(call_exit);