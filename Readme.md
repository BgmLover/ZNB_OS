### 关于NB_OS操作系统运行的一些说明



1. 关于本操作系统的具体测试指令说明

   在整体上，我们的shell提供的指令基本模仿linux指令，可能在参数的设定上有一些变化，以下是经过我们验证后推荐您测试的指令，这些指令的运行结果我们都有在**《操作系统运行实例》**报告中显示，如有错误，请您加以对照。

   * 涉及到文件部分的指令，请注意当前在根目录时请在文件名前加上 '/'


   * touch   创建文件，参数附上文件名即可
   * ls   查看当前目录下的文件，在根目录时参数为 '/'，在其他目录是参数为 ' '
   * vi   文本编辑
   * cat 文件查看
   * rm  文件删除
   * mkdir  文件夹创建
   * cd    改变目录
   * pwd   显示当前目录的路径
   * ps      显示当前操作系统中进程的信息（前后台队列信息，pid，name，state）
   * kill      参数需要给出一个具体的数字，该命令将杀死对应pid的进程
   * exec   外部加载程序的命令，第一个参数为文件名（注意是否在根目录），第二个参数为你想要给进程起的名字，涉及到外部程序加载的时候有可能会因为硬件或是编译器的原因导致bug，您在使用时请注意查看exec命令打印的调试信息中的The first instruction is   。。。。。这一句调试信息，这会打印程序的第一条指令的机器代码，如果您核实后确认无误，那这里应该就是硬件的问题了，在我们遇到的这样的情况中如果有问题基本都是出在这里。
   * clear   清屏   
   * echo    打印文字

2. 其它说明

   * 如果您在具体使用过程中遇到了其它的问题，请及时来联系我们

   ​