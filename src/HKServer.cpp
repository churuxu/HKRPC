#include "HKServer.h"
#include "Utils.h"
#include <mutex>

#define MAX_CONNECTION 8
#define MAX_EVENTS MAX_CONNECTION*2
#define BUF_SIZE 4096

#define CONN_TIMEOUT -1


#define TRACE printf

class Connection {
public:
	int index_;
	OVERLAPPED overlapr_;
	OVERLAPPED overlapw_;
	HANDLE pipe_;
	BOOL connected_;
	BOOL sending_;
	DWORD readed_;
	DWORD writed_;
	DWORD towrite_;
	char buffer_[BUF_SIZE];
	std::list<Buffer> sendqueue_;
	Buffer sendbuf_;
	std::mutex lock_;

	Connection(int index, HANDLE pipe, HANDLE revent, HANDLE wevent) {
		memset(&overlapr_, 0, sizeof(OVERLAPPED));
		memset(&overlapw_, 0, sizeof(OVERLAPPED));
		index_ = index;
		pipe_ = pipe;
		overlapr_.hEvent = revent;
		overlapw_.hEvent = wevent;
		connected_ = FALSE;
		readed_ = 0;
		writed_ = 0;
		towrite_ = 0;
		sending_ = FALSE;
	}
};


#define WM_WAIT_OBJECT  (WM_USER+1)
#define WM_SEND_DATA  (WM_USER+2)
#define WM_READY  (WM_USER+3)

class HKServerImpl :public HKServer {
protected:
	Connection* conns_[MAX_CONNECTION];
	HANDLE hevents_[MAX_EVENTS];
	ConnectionHandler onopen_;
	ConnectionHandler onclose_;
	ConnectionDataHandler ondata_;
	HWND handler_;
	
public:
	HKServerImpl() {
		memset(conns_, 0, sizeof(conns_));
		memset(hevents_, 0, sizeof(hevents_));
		CreateInstances();
	}
	~HKServerImpl() {
		DeleteInstances();
	}

	virtual void setOnConnect(ConnectionHandler handler) {
		onopen_ = handler;
	}

	virtual void setOnClose(ConnectionHandler handler) {
		onclose_ = handler;
	}

	virtual void setOnData(ConnectionDataHandler handler) {
		ondata_ = handler;
	}

	virtual void sendData(ConnectionPtr connid, const Buffer& data) {
		Connection* conn = conns_[connid];
		if (!conn)return;
		conn->lock_.lock();
		conn->sendqueue_.push_back(data);
		conn->lock_.unlock();

		//if (connid >= 0 && connid < MAX_CONNECTION && data.length()) {
		PostMessage(handler_, WM_READY, (WPARAM)this, (LPARAM)connid);
		//}
	}

	bool CreateInstances() {
		String exename = Utils::GetExecutableName();
		String pipename = "\\\\.\\pipe\\hkrpc\\" + exename;
		HANDLE hpipe;
		for (int i = 0; i < MAX_EVENTS; i++) {
			hevents_[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (!hevents_)return false;
		}
		for (int i = 0; i < MAX_CONNECTION; i++) {
			hpipe = CreateNamedPipeA(pipename.c_str(), FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, MAX_CONNECTION, BUF_SIZE, BUF_SIZE, CONN_TIMEOUT, NULL);
			if (hpipe == INVALID_HANDLE_VALUE)return false;
			conns_[i] = new Connection(i, hpipe, hevents_[i], hevents_[i + MAX_CONNECTION]);
			//StartAccept(conns_[i]);
		}
		for (int i = 0; i < MAX_CONNECTION; i++) {
			StartAccept(conns_[i]);
		}
		TRACE("create pipe server %s\n", pipename.c_str());
		return true;
	}

	void DeleteInstances() {

	}

	void OnError(Connection* conn, int errcode, const char* msg) {
		TRACE("connection %d error %d %s\n", conn->index_, errcode, msg);
		if (conn->connected_) {
			if (onclose_)onclose_(conn->index_);
			if (DisconnectNamedPipe(conn->pipe_)) {
				conn->connected_ = FALSE;
				StartAccept(conn);
			}
		}

	}


	bool StartAccept(Connection* conn) {		
		BOOL bret = ConnectNamedPipe(conn->pipe_, &conn->overlapr_);
		if (bret) {
			//已经连接上
			TRACE("client connected %d\n", conn->index_);
			conn->connected_ = TRUE;
			if (onopen_)onopen_(conn->index_);
			StartRead(conn);
		}
		else {
			if (GetLastError() == ERROR_IO_PENDING) {
				//连接中
			}
			else {
				//连接失败
				OnError(conn, GetLastError(), "ConnectNamedPipe error");
				return false;
			}
		}
		return true;
	}

	bool StartWrite(Connection* conn, DWORD remainlen = 0) {
		const char* packdata = nullptr;
		conn->lock_.lock();
		if (remainlen || conn->sendqueue_.size()) { //上次未发送完，或者发送队列里有数据
			conn->sending_ = true;
			packdata = conn->sendbuf_.c_str() + remainlen;
			if (!remainlen) {
				conn->sendbuf_ = conn->sendqueue_.front();
				conn->sendqueue_.pop_front();
				remainlen = conn->sendbuf_.length();
				packdata = conn->sendbuf_.c_str();
			}

			conn->towrite_ = remainlen;
			conn->writed_ = 0;
		}
		conn->lock_.unlock();

		if(packdata){
			BOOL bret = WriteFile(conn->pipe_, packdata, conn->towrite_, &conn->writed_, &conn->overlapw_);
			if (bret) {
				//发送直接完成
				StartWrite(conn, conn->towrite_ - conn->writed_);
			}
			else {
				if (GetLastError() == ERROR_IO_PENDING) {
					//发送中
				}
				else {
					//发送失败
					OnError(conn, GetLastError(), "WriteFile error");
					return false;
				}
			}
		}
		conn->sending_ = false;
		return true;
	}


	bool StartRead(Connection* conn) {
		BOOL bret = ReadFile(conn->pipe_, conn->buffer_, BUF_SIZE, &conn->readed_, &conn->overlapr_);
		if (bret) {
			//直接读取到数据
			if (ondata_) {
				Buffer buf;
				buf.assign(conn->buffer_, conn->readed_);
				ondata_(conn->index_, buf);
			}
			StartRead(conn);
		}
		else {
			if (GetLastError() == ERROR_IO_PENDING) {
				//读取中
			}
			else {
				//读取失败
				OnError(conn, GetLastError(), "ReadFile error");
				return false;
			}
		}
		return true;
	}


	void DoWaitEvents() {	
		while (1) {
			DWORD ret = WaitForMultipleObjects(MAX_EVENTS, hevents_, FALSE, INFINITE);
			SendMessage(handler_, WM_WAIT_OBJECT, (WPARAM)this, ret);
		}

	}


	void OnThreadWaited(DWORD ret) {
		int index;
		Connection* conn;
		BOOL bret;
		DWORD len = 0;

		index = (ret - WAIT_OBJECT_0);
		if (index < 0 || index >(MAX_EVENTS - 1)) {
			//Sleep(30);
			return;
		}
		if (index >= MAX_CONNECTION) {
			//写入完成事件触发
			index -= MAX_CONNECTION;
			conn = conns_[index];

			conn->writed_ = 0;
			bret = GetOverlappedResult(conn->pipe_, &conn->overlapw_, &conn->writed_, FALSE);
			if (!bret ) {
				//出错
				OnError(conn, GetLastError(), "Write Overlaped Result Error");
				return;
			}
			StartWrite(conn, conn->towrite_ - conn->writed_); //继续写数据
		}
		else {
			conn = conns_[index];
			if (!conn->connected_) {
				//未连接时，激活事件 = 连接成功
				conn->connected_ = TRUE;
				TRACE("client connected %d\n", conn->index_);
				if (onopen_)onopen_(index);
				StartRead(conn);
			}
			else {
				//已连接时，激活事件 = 读数据完成
				bret = GetOverlappedResult(conn->pipe_, &conn->overlapr_, &len, FALSE);
				if (!bret || len <= 0) {
					//出错
					OnError(conn, GetLastError(), "Read Overlaped Result Error");
					return;
				}
				if (ondata_) {
					Buffer buf;
					buf.assign(conn->buffer_, len);
					ondata_(index, buf);
				}
				StartRead(conn); //继续读数据
			}
		}
	}


	void OnQueueData(int connid, const Buffer* data) {
		Connection* conn = conns_[connid];
		conn->sendqueue_.push_back(*data);
		if (!conn->sending_) {
			//SetEvent(conn->overlapw_.hEvent);
			StartWrite(conn);
		}
	}

	static LRESULT CALLBACK MainThreadWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
		if (msg == WM_WAIT_OBJECT) {
			HKServerImpl* pthis = (HKServerImpl*)wp;
			pthis->OnThreadWaited(lp);
		}else if (msg == WM_READY) {
			HKServerImpl* pthis = (HKServerImpl*)wp;
			int connid = (int)lp;
			Connection* conn = pthis->conns_[connid];
			pthis->StartWrite(conn);
		}
		else if (msg == WM_SEND_DATA) {
			HKServerImpl* pthis = (HKServerImpl*)HKServer::getInstance();
			int connid = wp;
			Buffer* data = (Buffer*)lp;
			pthis->OnQueueData(connid, data);
		}
		else {
			return DefWindowProc(hwnd, msg, wp, lp);
		}
		return 1;
	}

	static DWORD CALLBACK HKWorkThread(PVOID arg) {
		HKServerImpl* pthis = (HKServerImpl*)arg;
		pthis->DoWaitEvents();
		return 1;
	}

	void start() {
		HINSTANCE hinst = GetModuleHandle(NULL);
		WNDCLASS wc = { 0 };
		wc.lpszClassName = _T("async_service_handler");
		wc.lpfnWndProc = MainThreadWndProc;
		wc.hInstance = hinst;
		RegisterClass(&wc);
		handler_ = CreateWindow(_T("async_service_handler"), NULL, 0, 0, 0, 0, 0, NULL, NULL, hinst, NULL);
		if (!handler_) {
			TRACE("CreateWindow Error %d\n",GetLastError());
		}
		else {
			TRACE("CreateWindow OK \n");
		}
		HANDLE th = CreateThread(NULL,0, HKWorkThread,this,0,NULL);
		if (!th) {
			TRACE("CreateThread Error %d\n", GetLastError());
		}
		else {
			TRACE("CreateThread OK \n");
		}

	}
	void stop() {

	}

	void close(ConnectionPtr connid) {		
		//OnError(conns_[connid], 0, "close");
	}
};







HKServer* HKServer::getInstance() {
	static HKServer* inst;
	if (!inst)inst = new HKServerImpl();
	return inst;
}



