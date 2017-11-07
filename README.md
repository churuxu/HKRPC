# HKRPC

提供远程进程hook等接口

# 基本流程

1. 注入HRRPC.dll到目标进程
2. 连接NamedPipe到 ``` \\.\pipe\hkrpc\<目标进程名> ```
3. 通过NamedPipe通信


# 消息包格式
```
packet{
    uint16 magic = 0xF8FB;
    uint16 jsonlen;
    char json[jsonlen];
}
```

# 接口说明
以下 --> 表示客户端发给服务端， <-- 表示服务端发给客户端

### 回显内容

- 参数：任意值
- 返回：同参数值
```
--> { "method": "echo", "params": ["hello","world"], "id": "1"}
<-- { "result": ["hello","world"], "id": "1"}
```

### 创建hook，监听目标函数|内存地址调用

- 参数：模块名，函数名|整数地址，内存数据类型表，内存地址表达式(可选)
- 返回：hook id
```
--> { "method": "hook", "params": ["user32.dll", "MessageBoxA", ["intptr","string","string","int"],["[esp+4]","[esp+8]","[esp+12]","[esp+16]"]], "id": "OnMessageBox"} 
<-- { "result": 321321, "id": "OnMessageBox"}  
```
 
### 通知hook被调用 

- 参数：调用该函数的参数值
```
<-- { "method": "hook_notify", "params": [642618,"hello","world",0], "requestid":"OnMessageBox"} 
```

### 取消hook

- 参数：hook id
```
--> { "method": "hook_delete", "params": [321321], "id": "111"} 
<-- { "result": "ok", "id": "111"}  
```

### 调用目标函数|内存地址

- 参数：模块名，函数名|整数地址，调用约定，参数表，参数类型表(可选)，返回类型(可选)
- 返回：调用该函数的返回值
```
--> { "method": "call", "params": ["user32.dll", "MessageBoxA","stdcall",[null,"hello","world",0], ["intptr","string","string","int"],"int"], "id": "111"} 
<-- { "result": 0, "id": "111"} 
```

### 获取模块版本

- 参数：模块名
- 返回：版本号字符串
```
--> { "method": "module_version", "params": ["user32.dll"], "id": "111"} 
<-- { "result": "6.0.1.1600", "id": "111"}  
```






