# winLockTestSrv
Windows 环境下通过服务的方式监听锁屏事件；
1、进入到执行文件LockTestSrv.exe 的目录；
2、执行LockTestSrv.exe -create命令，将创建名称为Lock的服务；
3、启动此服务；
4、进行windows系统锁屏，在C盘目录下将生成LockTestSrvLog.txt的文件，且文件内容中将出现"Locking start ."
