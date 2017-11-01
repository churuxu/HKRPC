var net = require("net");

var pipename = "\\\\.\\pipe\\hkrpc\\test-server.exe";

function makePacket(str){
	var buffer1 = new Buffer(4);
	var buffer2 = new Buffer(str); 
	buffer1.writeUInt16BE(0xF8FB);
	buffer1.writeUInt16BE(buffer2.length, 2);
	console.log("[SEND]\n "+ str);
	return Buffer.concat([buffer1, buffer2]);
}

function testEcho(client){
	var pack = {};	
	pack.id = "echo1";
	pack.method = "echo";
	pack.params = [];
	pack.params.push("aaaa");
	pack.params.push(111);
	pack.params.push(true);	
	
	client.write(makePacket(JSON.stringify(pack)));	
}

function testHook1(){
	var pack = {};		
	pack.id = "hook1";
	pack.method = "hook";
	pack.params = [];
	pack.params.push("user32.dll");
	pack.params.push("MessageBoxA");	
	var types = [];
	types.push("intptr");
	types.push("string");	
	types.push("string");	
	types.push("int");
	pack.params.push(types);	
	
	client.write( makePacket(JSON.stringify(pack)));	
}

function testHook2(){
	var pack = {};	
	pack.id = "hook2";
	pack.method = "hook";
	pack.params = [];
	pack.params.push("user32.dll");
	pack.params.push("MessageBoxW");	
	var types = [];
	types.push("intptr");
	types.push("wstring");	
	types.push("wstring");	
	types.push("int");
	pack.params.push(types);	
	
	client.write( makePacket(JSON.stringify(pack)));	
}

function sendSetWindowText(client, hwnd, text){
	var pack = {};	
	pack.id = "call1";
	pack.method = "call";
	pack.params = [];
	pack.params.push("user32.dll");
	pack.params.push("SetWindowTextW");	
	pack.params.push("stdcall");
	var args = [];
	args.push(hwnd);
	args.push(text);
	var types = [];
	types.push("intptr");
	types.push("wstring");	
	pack.params.push(args);
	pack.params.push(types);
	client.write( makePacket(JSON.stringify(pack)));	
}

var count = 0;
function onRecvPacket(client, str){
	console.log("[RECV]\n " +str);
	
	var obj = JSON.parse(str);
	if(obj.method == "hook_notify" && obj.requestid == "hook1"){
		var hwnd = obj.params[0];
		count++;
		sendSetWindowText(client, hwnd, "点击" + count + "次");
	}
}

var client = net.connect(pipename, function() {
    console.log('connection ok');
	
	testEcho(client);
	testHook1(client);
	testHook2(client);
})


var packetbuf = null;
function decodePacket(client, buf){
	var remainlen = buf.length; 
	var packetstart = 0;
	var haspack = false;
	while(remainlen > 4){		
		var packlen = buf.readUInt16BE(packetstart + 2);
		packlen += 4;
		if(remainlen >= packlen){ 
            var data = packetbuf.toString("utf8", packetstart + 4, packetstart + packlen );		
			onRecvPacket(client, data);
			remainlen -= packlen;
			packetstart += packlen;
			haspack = true;
		}else{
			break;
		}
	}
	if(haspack){
		return buf.slice(buf.length - remainlen);
	}
	return buf;		
}

client.on('data', function(data) {
	if(packetbuf && packetbuf.length){
		packetbuf = Buffer.concat([packetbuf, data]);
	}else{
		packetbuf = new Buffer(data);
	}
	if(packetbuf.length < 4)return;
	packetbuf = decodePacket(client, packetbuf);   
});

client.on('end', function() {
    console.log('on close');
})
client.on('error', function(e) {
    console.log('error\n'+e.stack);
})

