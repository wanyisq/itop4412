# 1. 工作队列和tasklet的区别
# A.工作队列将要推后的工作交给一个进程执行，工作在进程上下文，因此允许休眠和重新调度。
# B.tasklet是通过软中断实现的，在软中断上下文切换，代码必须是原子的