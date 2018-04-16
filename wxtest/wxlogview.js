/**
微信实时日志查看(2.6.2.27)
*/

var net = require("net");

var pipename = "\\\\.\\pipe\\hkrpc\\WeChat.exe";

function makePacket(str){
	var buffer1 = new Buffer(4);
	var buffer2 = new Buffer(str); 
	buffer1.writeUInt16BE(0xF8FB);
	buffer1.writeUInt16BE(buffer2.length, 2);
	//console.log("[SEND]\n "+ str);
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

function getModuleInfo(client,mod){
	var pack = {};	
	pack.id = "minfo";
	pack.method = "module_info";
	pack.params = [];
	pack.params.push(mod);
	
	client.write(makePacket(JSON.stringify(pack)));	
}

function hookLog(){
	var pack = {};	
	pack.id = "log";
	pack.method = "hook";
	pack.params = [];
	pack.params.push("WeChatWin.dll");
	pack.params.push(0x40F4EA);	
	var types = ["string"];
	var exps = ["[ebp-4568]"];
	
	//var types = ["lstring"];
	//var exps = ["[ebp-4564]"];	
	pack.params.push(types);	
	
	pack.params.push(exps);
	
	client.write( makePacket(JSON.stringify(pack)));	
}






function onRecvPacket(client, str){
	//console.log("[RECV]\n " +str);
	
	var obj = JSON.parse(str);
	if(obj.method == "hook_notify" && obj.requestid == "log"){
		var str = obj.params[0];
		var lsti = str.lastIndexOf("\n");
		if(lsti>0){
			str = str.substr(0,lsti);
		}
		
		console.log(str);
		
		//sendSetWindowText(client, hwnd, "点击" + count + "次");
	}
}



var client = net.connect(pipename, function() {
    console.log('connection ok');
	
	//testEcho(client);
	getModuleInfo(client, "WeChatWin.dll");
	//hookAddFriend(client)
	hookLog(client)
	
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

