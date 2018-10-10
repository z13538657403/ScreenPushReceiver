# ScreenPushReceiver
无线传屏Android接收端

无线传屏接收端实现思路：在JNI层进行数据接收，这里需要在JNI层开启一个子线程，使用死循环不断接收发送端发送过来的数据。这里有一个难点就是需要对接收到的数据进行组包。

组包成一帧帧完整的数据在JNI层调用Java层的解码方法，将H264数据传递到Java层解码播放，具体实现请看源码。

发送端源码：https://github.com/z13538657403/ScreenPushSender
