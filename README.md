# 一个简单的代理服务器

当前支持 **火狐(Firefox)** 游览器

部署暂时只支持linux

支持https与http

代理服务器可以级联使用

即 client->proxy->...->proxy->server

## 使用方法

在火狐游览器的设置里勾选代理，设置ip端口即可

最好采用本地一个代理连远程一个代理的方式即

client -> proxy -> proxy -> server

远程代理设置proxy2server

设置ip 0.0.0.0 和开放的端口

本地代理设置client2proxy

设置本机ip 0.0.0.0 和本机开放的端口


设置代理 {远程代理ip} {远程代理的端口}

然后游览器设置 本机ip 和本机开放的端口即可上网

具体配置见conf中的配置文件


