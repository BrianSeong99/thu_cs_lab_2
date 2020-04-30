 // ubuntu 16
 #include <linux/init.h>
 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/unistd.h>
 #include <asm/uaccess.h>
 #include <linux/sched.h>
 
 //去系统调用表中查找一个空闲的系统调用号
 //使用已经存在的调用号也ok
#define SYSMYCALL 329  //系统调用号为223
#define SYSMYCALLA 330 //如下的这个就是上一步获得的值
 #define sys_call_table_adress 0xffffffff81a00240
 
 unsigned int clear_and_return_cr0(void);//清除写保护位并返回
 void setback_cr0(unsigned int val);//恢复写保护位
 static void sys_mycall(size_t, size_t);//自定义的系统调用函数
 static size_t sys_mycallA(sizea_t);//自定义的系统调用函数
 
uint8_t array1[160] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
size_t array1_size = 16;

uint8_t array2_temp[256 * 512];
uint8_t *array2;
uint8_t temp;

 int orig_cr0;
 unsigned long *sys_call_table = 0;
 static int (*anything_saved)(void);//函数指针
 static int (*anything_savedA)(void);//函数指针
 
 unsigned int clear_and_return_cr0(void)
 {
     unsigned int cr0 = 0;
     unsigned int ret;
     asm("movq %%cr0, %%rax":"=a"(cr0));
     ret = cr0;
     cr0 &= 0xfffffffffffeffff;
     asm("movq %%rax, %%cr0"::"a"(cr0));
     return ret;
 }
 
 void setback_cr0(unsigned int val) //读取val的值到eax寄存器，再将eax寄存器的值放入cr0中
 {
     asm volatile("movq %%rax, %%cr0"::"a"(val));
 }
 
 static int __init init_addsyscall(void)
 {
     printk("hello, kernel\n");
 
     sys_call_table = (unsigned long *)sys_call_table_adress;//获取系统调用服务首地址
     anything_saved = (int(*)(void))(sys_call_table[SYSMYCALL]);//保存原始系统调用的地址
     anything_savedA = (int(*)(void))(sys_call_table[SYSMYCALLA]);//保存原始系统调用的地址
  
     orig_cr0 = clear_and_return_cr0();//设置cr0可更改
     sys_call_table[SYSMYCALL] = (unsigned long)&sys_mycall;//更改原始的系统调用服务地址 
     sys_call_table[SYSMYCALLA] = (unsigned long)&sys_mycallA;//更改原始的系统调用服务地址 
     setback_cr0(orig_cr0);//设置为原始的只读cr0
 
     return 0;
 }
 
 static void sys_mycall(size_t secret_offset, size_t prob_offset)
 {
     volatile int z;
     array2 = array2_temp + array1[secret_offset] * 4096 + prob_offset;
     for(z = 0; z<100;z++){}
     asm volatile("clflush 0(%0)\n" : : "c"(&array1_size) : "eax");
     for(z = 0; z<100;z++){}
    asm volatile("clflush 0(%0)\n" : : "c"(&secret_offset) : "eax");
     for(z = 0; z<100;z++){}

    if (secret_offset < array1_size)
        temp = *array2;
 }

 static size_t sys_mycallA(size_t physical_address) {
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
    vir_address = phys_to_virt(physical_address); // + 0xffff880000000000;
    printk("CZY: virtual address is 0x%p\n", vir_address);
    return (size_t)vir_address;
}
 static void __exit exit_addsyscall(void)
 {
     //设置cr0中对sys_call_table的更改权限。
     orig_cr0 = clear_and_return_cr0();//设置cr0可更改
 
     //恢复原有的中断向量表中的函数指针的值。
     sys_call_table[SYSMYCALL] = (unsigned long)anything_saved;
     sys_call_table[SYSMYCALLA] = (unsigned long)anything_savedA;
  
     //恢复原有的cr0的值
     setback_cr0(orig_cr0);
 
     printk("call exit \n");
 }
 
 module_init(init_addsyscall);
 module_exit(exit_addsyscall);
 MODULE_LICENSE("GPL");
