# 1. tasklet uasge:

## 1. 定义使用
```
/* 定义 taselet
*/
struct tasklet_struct testtasklet;
/* tasklet 处理函数
*/
void testtasklet_func(unsigned long data)
{
    /* tasklet 具体处理内容 */
}
/* 中断处理函数 */
irqreturn_t test_handler(int irq, void *dev_id)
{
    ......
    /* 调度 tasklet
    */
    tasklet_schedule(&testtasklet);
    ......
}
/* 驱动入口函数
*/
static int __init xxxx_init(void)
{
    ......
    /* 初始化 tasklet
    */
    tasklet_init(&testtasklet, testtasklet_func, data);
    /* 注册中断处理函数
    */
    request_irq(xxx_irq, test_handler, 0, "xxx", &xxx_dev);
    ......
}
```
## 2. 使用宏定义
```
DECLARE_TASKLET(name, func, data)
```
其中name为要定义的tasklet名字， 这个名字就是tasklet_struct类型的变量，func是tasklet的处理函数，data是传给tasklet的变量。
然后在使用是调用
```
tasklet_schedule(&name);
```