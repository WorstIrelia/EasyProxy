1. connection: close 
    请求体中带connection: close的报文要等待所有报文体到达之后在发送。
2. unknown里是user-agent
3. epoll_mod里 crash
4. 奇怪的crash 0377(貌似没了)                                                                                          已经解决
5. assert ./src/auto_match.c:144: _auto_match_data_body: Assertion `n == 0' failed.                                  级联的时候会失败
6. 从链接池中取出的链接已经失效，但是还是关联起来，最后导致读server直接失效
7. send 阻塞？ netstat可以看吗 实验                                                                                     
8. qipashuo content-length = 0 但是还带了实体? 逻辑bug?                                                                 已修复，采取截断。
9. 服务器出现大量的close_wait状态，原因暂时未知。                                                                          已修复，在链接池中的链接应该继续监听，等待关闭。
10. 出现了一次奇怪的BROKEN_PIPE                                                                                         未修复
11. 出现了大量的send_SYN                                                                                                链接不上的优化, 天天连google啥的。
12. send 和 read的 buf                                                    动态增大，减少系统调用
13. read n == 0 进入了最后一个分支 导致fail                                                                              未修复



重构一下代码，有些地方逻辑有些混乱