#include "Program.h"

Program::Program()
{
	FixSizeWindow(110, 30);
	FixConsoleWindow();

	/* GUI */
	this->selected = SELECTED::REGISTER;
	this->line_2 = 0;
	this->line_3 = 2;

	// Init something you need
	this->initDataBaseDirectory();
	this->initFileList();

	/* LOG */
	fstream f(DATABASE_PATH + "\\" + LOG_FILE, std::fstream::out | std::fstream::app);
	time_t now = time(0);
	tm* ltm = localtime(&now);
	f << "================ Date: " << ltm->tm_mday << " - " << 1 + ltm->tm_mon << " - " << (1900 + ltm->tm_year) << " ================" << endl;
	f.close();
}

Program::~Program()
{
	// ...
}

void Program::run()
{
	this->homeScreen();
	/*std::thread userInteractThread(&Program::homeScreen, this);
	userInteractThread.detach();

	this->initWinsock();
	this->initConnectSocket();

	std::thread rcvMsgThread(&Program::receiveMsg, this);
	rcvMsgThread.join();*/
}

void Program::initDataBaseDirectory() {
	if (CreateDirectory(s2ws(DATABASE_PATH).c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
		CreateDirectory(s2ws(DATABASE_PATH + "\\" + DOWNLOAD_FOLDER).c_str(), NULL);
	}
	else return;
}

void Program::initFileList() {
	// ...
}

void Program::initWinsock()
{
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		this->LastError = "WSAStartup() failed with error: " + std::to_string(iResult);
		this->printLastError();
	}
}

void Program::initConnectSocket()
{
	struct addrinfo* result = nullptr;	// A pointer to a linked list of one or more addrinfo structures that contains response information about the host.
	struct addrinfo hints;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(this->ServerIP, this->SERVER_PORT, &hints, &result);	// Update 'result' with port, IP address,...

	if (iResult != 0) {
		this->LastError = "getaddrinfo() failed: " + std::to_string(iResult);
		this->printLastError();
		return;
	}

	this->UserInfo.ConnectSocket = INVALID_SOCKET;

	for (auto ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		this->UserInfo.ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (this->UserInfo.ConnectSocket == INVALID_SOCKET) {
			this->LastError = "socket() failed with error: " + std::to_string(WSAGetLastError());
			this->printLastError();
			return;
		}

		iResult = connect(this->UserInfo.ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

		if (iResult == SOCKET_ERROR) {
			closesocket(this->UserInfo.ConnectSocket);
			this->UserInfo.ConnectSocket = INVALID_SOCKET;
			continue;
		}

		break;
	}

	if (this->UserInfo.ConnectSocket == INVALID_SOCKET) {
		this->LastError = "Error at socket(): " + std::to_string(WSAGetLastError());
		this->printLastError();
	}

	freeaddrinfo(result);
}

void Program::receiveMsg()
{
	/* Message structure: FLAG (uint8_t) | MSGLEN (uint64_t) | MSG (char*) */

	int shutdownFlag;

	RcvMsgFlag flag;
	uint64_t msgLen;
	char* msg;

	while (true) {
		shutdownFlag = this->receiveData((char*)&flag, sizeof(flag));
		if (shutdownFlag == 0)	// Check if the Server shutdowns
			break;

		this->receiveData((char*)&msgLen, sizeof(msgLen));
		msg = new char[msgLen + 1];

		this->receiveData(msg, msgLen);
		msg[msgLen] = '\0';

		switch (flag)
		{
		case RcvMsgFlag::FAIL: {
			// ...

			break;
		}
		case RcvMsgFlag::SUCCESS: {
			// ...

			break;
		}
		case RcvMsgFlag::DOWNLOAD_FILE: {
			std::string downloadPath = this->DATABASE_PATH + "\\" + this->DOWNLOAD_FOLDER + "\\" + this->FileList[this->line_2].fileName;	// default path
			printLog(flag);
			this->receiveADownloadFileReply(downloadPath);
			break;
		}
		case RcvMsgFlag::LOGOUT: {
			// ...

			break;
		}
		default:
			break;
		}

		delete[] msg;
		msg = nullptr;
	}
}

void Program::sendMsg(SendMsgFlag const& flag, const char* msg, uint64_t const& msgLen)
{
	this->sendData((char*)&flag, sizeof(flag));
	this->sendData((char*)&msgLen, sizeof(msgLen));
	this->sendData(msg, msgLen);
}

int Program::receiveData(char* buffer, uint64_t const& len)
{
	int iResult;

	iResult = recv(this->UserInfo.ConnectSocket, buffer, len, 0);
	if (iResult == SOCKET_ERROR) {
		this->LastError = "recv() failed with error: " + std::to_string(WSAGetLastError());
		this->printLastError();
	}

	return iResult;
}

int Program::sendData(const char* buffer, uint64_t const& len)
{
	int iResult;

	iResult = send(this->UserInfo.ConnectSocket, buffer, len, 0);
	if (iResult == SOCKET_ERROR) {
		this->LastError = "send() failed with error: " + std::to_string(WSAGetLastError());
		this->printLastError();
	}

	return iResult;
}

void Program::registerAccount() {
	string username;
	string password;
	uint8_t flag; // Placeholder
	uint64_t msgLen;
	char* msg;
	bool result = false;

	while (result == false) {
		cout << "Username: ";	getline(cin, username);
		cout << "Password: ";	getline(cin, password);

		// Send account info for registration
		flag = 0; // Placeholder
		send(this->UserInfo.ConnectSocket, (char*)&flag, sizeof(flag), 0);
		msgLen = username.length();
		send(this->UserInfo.ConnectSocket, (char*)&msgLen, sizeof(msgLen), 0);
		send(this->UserInfo.ConnectSocket, (char*)username.c_str(), msgLen, 0);
		flag = 2; // Placeholder
		send(this->UserInfo.ConnectSocket, (char*)&flag, sizeof(flag), 0);
		msgLen = password.length();
		send(this->UserInfo.ConnectSocket, (char*)&msgLen, sizeof(msgLen), 0);
		send(this->UserInfo.ConnectSocket, (char*)password.c_str(), msgLen, 0);

		// Receive server's response
		recv(this->UserInfo.ConnectSocket, (char*)&flag, sizeof(flag), 0);

		switch (flag) {
		case 0:	// Placeholder	// Registration failed
		{
			recv(this->UserInfo.ConnectSocket, (char*)&msgLen, sizeof(msgLen), 0);
			msg = new char[msgLen + 1];
			memset(msg, 0, msgLen + 1);
			recv(this->UserInfo.ConnectSocket, (char*)msg, msgLen, 0);
			cout << "Registration failed." << endl;
			cout << msg << endl;
			delete[] msg;
			break;
		}
		case 1:	// Placeholder	// Registration succeeded
		{
			cout << "Registration succeeded." << endl;
			result = true;
			break;
		}
		default:
			cout << "Illegal flag. Something went wrong..." << endl;
		}
	}
}

void Program::sendADownloadFileRequest(uint64_t const& fileIndex)
{
	/* Message structure: FLAG (uint8_t) | MSGLEN (uint64_t) | MSG (string) */

	// Send a request to the Server first.
	SendMsgFlag flag = SendMsgFlag::DOWNLOAD_FILE;
	std::string msg = std::to_string(fileIndex);
	uint64_t msgLen = msg.length();

	printLog(flag);

	this->sendMsg(flag, (char*)msg.c_str(), msgLen);

	// Then, waiting for a reply from the Server and receive file.
}

void Program::receiveADownloadFileReply(std::string const& downloadedFilePath)
{
	std::ofstream fout(downloadedFilePath, std::ios_base::binary);

	if (fout.is_open()) {
		int iResult;
		
		uint64_t fileSize;
		char* buffer = new char[this->BUFFER_LEN];

		// Receive file's size
		this->receiveData((char*)&fileSize, sizeof(fileSize));

		printProgressBar(0);// Start 0%
		
		// Receive file's data
		for (uint64_t i = 0; i < fileSize / this->BUFFER_LEN; ++i) {
			this->receiveData(buffer, this->BUFFER_LEN);
			fout.write(buffer, this->BUFFER_LEN);

			//Progress
			printProgressBar((i + 1) * this->BUFFER_LEN / fileSize);
		}

		this->receiveData(buffer, fileSize % this->BUFFER_LEN);
		fout.write(buffer, fileSize % this->BUFFER_LEN);

		printProgressBar(1); // Complete 100%
		string gui = string("Download ") + shortenFileName(FileList[line_2].fileName) + string(" (") + FileList[line_2].fileSize + string(") succeed.");
		string log = string("Download ") + FileList[line_2].fileName + string(" (") + FileList[line_2].fileSize + string(") succeed.");
		printLog(gui, log);

		// Release resources
		delete[] buffer;
		fout.close();
	}
	else {
		this->LastError = "Unable to open file " + downloadedFilePath;
		this->printLastError();
	}
}

void Program::uploadFile(std::string const& uploadedFilePath)
{
	std::string fileName = this->getFileNameFromPath(uploadedFilePath);

	// Send a request with the corresponding flag first.
	SendMsgFlag flag = SendMsgFlag::UPLOAD_FILE;
	const char* msg = fileName.c_str();
	uint64_t msgLen = fileName.length();

	printLog(flag);
	this->sendMsg(flag, msg, msgLen);

	// Then, send the file to the Server.
	ifstream fin(uploadedFilePath, std::ios_base::binary);

	if (fin.is_open()) {
		int iResult;

		uint64_t fileSize;
		char* buffer = new char[this->BUFFER_LEN];

		// Get file's size
		fin.seekg(0, std::ios_base::end);
		fileSize = fin.tellg();
		fin.seekg(0, std::ios_base::beg);

		// Send file's size
		this->sendData((char*)&fileSize, sizeof(fileSize));

		printProgressBar(0); // Start 0%

		// Send file's data
		for (uint64_t i = 0; i < fileSize / this->BUFFER_LEN; ++i) {
			fin.read(buffer, this->BUFFER_LEN);
			this->sendData(buffer, this->BUFFER_LEN);

			//Progress
			printProgressBar((i + 1) * this->BUFFER_LEN / fileSize);
		}
		fin.read(buffer, fileSize % this->BUFFER_LEN);
		this->sendData(buffer, fileSize % this->BUFFER_LEN);

		printProgressBar(1); // Complete 100%
		string gui = string("Upload ") + shortenFileName(FileList[line_2].fileName) + string(" (") + FileList[line_2].fileSize + string(") succeed.");
		string log = string("Upload ") + FileList[line_2].fileName + string(" (") + FileList[line_2].fileSize + string(") succeed.");
		printLog(gui, log);

		// Release resources
		delete[] buffer;
		fin.close();

		fin.close();
	}
	else {
		this->LastError = "Unable to open file " + uploadedFilePath;
		this->printLastError();
	}
}

std::string Program::getFileNameFromPath(std::string const& path)
{
	size_t startPos = 0;
	for (int i = path.length(); i >= 0; --i) {
		if (path[i] == '\\' || path[i] == '/') {
			startPos = i + 1;
			break;
		}
	}

	return path.substr(startPos, path.length() - startPos);
}

void Program::printLastError()
{
	cout << this->LastError << "\n";
}


void Program::homeScreen() {
	this->printTitle();
	this->printMode();
	this->printClient();
	this->navigateClient();


	for (int i = 0; i < FileList.size(); i++) {
		printFile(shortenFileName(FileList[i].fileName), FileList[i].fileSize, false);
		line_2++;
	}
	line_2 = 2;
	
	this->navigateMode();
}

void Program::printMode() {
	if (selected == SELECTED::DOWNLOAD) {
		setColor(COLOR::BLACK, COLOR::DARK_GRAY);
		gotoXY(30, 0);
		cout << " DOWNLOAD ";
	}
	else {
		setColor(COLOR::BLACK, COLOR::LIGHT_CYAN);
		gotoXY(30, 0);
		cout << "  UPLOAD  ";
	}
}

void Program::navigateMode() {
	selected = SELECTED::UPLOAD;
	bool esc = false;

	while (true) {

		if (_kbhit()) {
			FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

			// ============= LEFT/RIGHT =============
			if ((GetKeyState(VK_LEFT) & 0x8000) || (GetKeyState(VK_RIGHT) & 0x8000)) {

				if ((selected == SELECTED::UPLOAD) || (selected == SELECTED::DOWNLOAD)) {
					selected = (selected == SELECTED::UPLOAD) ? SELECTED::DOWNLOAD : SELECTED::UPLOAD;
					this->printMode();

					if (FileList.size() > 0) {
						printFile(FileList[0].fileName, FileList[0].fileSize, false); // reset the previous line back to normal
						line_2 = 0;									  // reset line back to the top of FileList
						if (selected == SELECTED::DOWNLOAD) {
							printFile(FileList[0].fileName, FileList[0].fileSize, true);
						}
					}
				}
				

				if ((selected == SELECTED::YES) || (selected == SELECTED::NO)) {
					selected = (selected == SELECTED::YES) ? SELECTED::NO : SELECTED::YES;
					this->printStatus();
				}
			}

			// ============= UP =============
			if (GetKeyState(VK_UP) & 0x8000) {
				if (selected == SELECTED::DOWNLOAD) {
					if (FileList.size() > 0) {
						printFile(FileList[line_2].fileName, FileList[line_2].fileSize, false); // reset the previous line back to normal
						line_2--;
						line_2 += (line_2 < 0) ? FileList.size() : 0;
						printFile(FileList[line_2].fileName, FileList[line_2].fileSize, true);
					}
				}
			}

			// ============= DOWN =============
			if (GetKeyState(VK_DOWN) & 0x8000) {
				if (selected == SELECTED::DOWNLOAD) {
					if (FileList.size() > 0) {
						printFile(FileList[line_2].fileName, FileList[line_2].fileSize, false); // reset the previous line back to normal
						line_2++;
						line_2 %= FileList.size();
						printFile(FileList[line_2].fileName, FileList[line_2].fileSize, true);
					}
				}
			}

			// ============= ENTER =============
			if (GetKeyState(VK_RETURN) & 0x8000) {

				if (selected == SELECTED::UPLOAD) {
					std::string uploadedFilePath = this->enterPath();
					this->uploadFile(uploadedFilePath);		// ... Suppose that 'uploadedFilePath' is valid.
				}

				if (selected == SELECTED::DOWNLOAD) {
					if (FileList.size() > 0) {
						this->sendADownloadFileRequest(this->line_2);	// ... Download at the default path.
					}
				}

				if (selected == SELECTED::YES) {
					esc = true;
				}

				if (selected == SELECTED::NO) {
					selected = SELECTED::UPLOAD;
					printStatus();
				}
			}

			// ============= ESC =============
			if (GetKeyState(VK_ESCAPE) & 0x8000) {
				if (FileList.size() > 0) {
					printFile(FileList[0].fileName, FileList[0].fileSize, false); // reset the previous line back to normal
					line_2 = 0;									  // reset line back to the top of FileList
				}
				selected = SELECTED::UPLOAD;
				this->printMode();

				selected = SELECTED::NO;
				this->printStatus();
			}
		}

		if (esc) break;
	}
}

string Program::enterPath() {
	string path;

	gotoXY(41, 0);
	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK);
	getline(cin, path);

	gotoXY(41, 0); printSpace(69);
	return path;
}

void Program::printTitle() {
	setColor(COLOR::BLACK, COLOR::LIGHT_GRAY);
	gotoXY(30, 1); printSpace(8); cout << " File Shared "; printSpace(8); 
	
	setColor(COLOR::BLACK, COLOR::LIGHT_GRAY);
	cout << "|"; 
	
	setColor(COLOR::BLACK, COLOR::LIGHT_GRAY);
	printSpace(20); cout << "History Log"; printSpace(19);


	setColor(COLOR::WHITE, COLOR::BLACK);
}

void Program::printFile(string name, string size, bool selected) {
	gotoXY(30, line_2 + 2);
	 
	if (selected) {
		setColor(COLOR::BLACK, COLOR::LIGHT_BLUE);
	}
	else setColor(COLOR::WHITE, COLOR::BLACK);
	cout << " " << name; printSpace(27 - name.length() - size.length()); cout << size << " ";
}

void Program::printLog(string gui, string log) {
	fstream f(DATABASE_PATH + "\\" + LOG_FILE, std::fstream::out | std::fstream::app);
	time_t now = time(0);
	tm* ltm = localtime(&now);
	string timeline;

	gotoXY(61, line_3);

	// [hh:mm]
	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK);
	if (ltm->tm_hour < 10) timeline = string("[0") + numCommas(ltm->tm_hour);
	else timeline = string("[") + numCommas(ltm->tm_hour);
	if (ltm->tm_min < 10) timeline += string(":0") + numCommas(ltm->tm_min) + string("] ");
	else timeline += string(":") + numCommas(ltm->tm_min) + string("] ");

	cout << timeline;
	f << timeline;

	// Content
	setColor(COLOR::WHITE, COLOR::BLACK);
	cout << gui;
	f << log << endl;

	line_3++;
	f.close();
}

void Program::printLog(SendMsgFlag flag) {
	fstream f(DATABASE_PATH + "\\" + LOG_FILE, std::fstream::out | std::fstream::app);
	time_t now = time(0);
	tm* ltm = localtime(&now);
	string timeline;

	gotoXY(61, line_3);

	// [hh:mm]
	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK);
	if (ltm->tm_hour < 10) timeline = string("[0") + numCommas(ltm->tm_hour);
	else timeline = string("[") + numCommas(ltm->tm_hour);
	if (ltm->tm_min < 10) timeline += string(":0") + numCommas(ltm->tm_min) + string("] ");
	else timeline += string(":") + numCommas(ltm->tm_min) + string("] ");

	cout << timeline;
	f << timeline;

	setColor(COLOR::WHITE, COLOR::BLACK);
	string content;
	
	switch (flag) {
	case SendMsgFlag::REGISTER:
		content = UserInfo.Username + string(" request to register.");
		break;
	case SendMsgFlag::LOGIN:
		content = UserInfo.Username + string(" request to login.");
		break;
	case SendMsgFlag::PASSWORD:
		break;
	case SendMsgFlag::UPLOAD_FILE:
		content = string("Request to upload ") + shortenFileName(FileList[line_2].fileName) + string(" (") + FileList[line_2].fileSize + string(").");
		cout << content;
		content = string("Request to upload ") + FileList[line_2].fileName + string(" (") + FileList[line_2].fileSize + string(").");
		f << content << endl;
		break;
	case SendMsgFlag::DOWNLOAD_FILE:
		content = string("Request to download ") + shortenFileName(FileList[line_2].fileName) + string(" (") + FileList[line_2].fileSize + string(").");
		cout << content;
		content = string("Request to download ") + FileList[line_2].fileName + string(" (") + FileList[line_2].fileSize + string(").");
		f << content << endl;
		break;

	case SendMsgFlag::LOGOUT:
		content = UserInfo.Username + string(" log out.");
		cout << content;
		f << content << endl;
		break;

	default:
		break;
	}

	line_3++;
	f.close();
}

void Program::printLog(RcvMsgFlag flag) {
	fstream f(DATABASE_PATH + "\\" + LOG_FILE, std::fstream::out | std::fstream::app);
	time_t now = time(0);
	tm* ltm = localtime(&now);
	string timeline;

	gotoXY(61, line_3);

	// [hh:mm]
	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK);
	if (ltm->tm_hour < 10) timeline = string("[0") + numCommas(ltm->tm_hour);
	else timeline = string("[") + numCommas(ltm->tm_hour);
	if (ltm->tm_min < 10) timeline += string(":0") + numCommas(ltm->tm_min) + string("] ");
	else timeline += string(":") + numCommas(ltm->tm_min) + string("] ");

	cout << timeline;
	f << timeline;

	setColor(COLOR::WHITE, COLOR::BLACK);
	string content;

	switch (flag) {
	case RcvMsgFlag::FAIL:
		break;
	case RcvMsgFlag::SUCCESS:
		break;

	case RcvMsgFlag::DOWNLOAD_FILE:
		content = string("Start downloading ") + shortenFileName(FileList[line_2].fileName) + string(" (") + FileList[line_2].fileSize + string(").");
		break;

	case RcvMsgFlag::LOGOUT:
		break;
	}

	cout << content;
	f << content << endl;

	line_3++;
	f.close();
}

void Program::buttonClient() {
	gotoXY(1, 10);
	if (selected == SELECTED::REGISTER) {
		setColor(COLOR::BLACK, COLOR::LIGHT_RED);
	}
	else setColor(COLOR::BLACK, COLOR::DARK_GRAY);
	cout << "  REGISTER  ";

	setColor(COLOR::WHITE, COLOR::BLACK);
	cout << " ";

	if (selected == SELECTED::LOGIN) {
		setColor(COLOR::BLACK, COLOR::LIGHT_RED);
	}
	else setColor(COLOR::BLACK, COLOR::DARK_GRAY);
	cout << "     LOGIN     ";

	setColor(COLOR::BLACK, COLOR::BLACK);
}

void Program::printClient() {
	gotoXY(0, 9); 
	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK);
	printTextAtMid("[CLIENT]", 0, 30);

	buttonClient();

	setColor(COLOR::DARK_GRAY, COLOR::BLACK);
	gotoXY(0, 11); cout << "|                            |";
	gotoXY(0, 12); cout << "|                            |";
	gotoXY(0, 13); cout << "|                            |";
	gotoXY(0, 14); cout << "'============================'";

	setColor(COLOR::DARK_GRAY, COLOR::BLACK);
	gotoXY(1, 11); cout << "ServerIP:";
	gotoXY(1, 12); cout << "Username:";
	gotoXY(1, 13); cout << "Password:";

}

void Program::navigateClient() {
	while (true) {

		if (_kbhit()) {
			FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

			// ============= LEFT =============
			if (GetKeyState(VK_LEFT) & 0x8000) {
				selected = (selected == SELECTED::REGISTER) ? SELECTED::LOGIN : SELECTED::REGISTER;
				this->buttonClient();

			}

			// ============= RIGHT =============
			if (GetKeyState(VK_RIGHT) & 0x8000) {
				selected = (selected == SELECTED::REGISTER) ? SELECTED::LOGIN : SELECTED::REGISTER;
				this->buttonClient();
			}

			// ============= ENTER =============
			if (GetKeyState(VK_RETURN) & 0x8000) {

				this->loginClient();

				if (selected == SELECTED::REGISTER) {
					// CSOCKET register
					
					// ...
					
					// After Register -> Login
					selected = SELECTED::LOGIN;
					this->buttonClient();
					this->printClient();
					this->loginClient();
				}
				
				if(selected == SELECTED::LOGIN) { 
					// CSOCKET login
					
					// ...
				}

				this->printStatus();

				break;
			}
		}

	}
}

void Program::loginClient() {
	std::string input;

	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK); gotoXY(1, 11); cout << "ServerIP: ";
	setColor(COLOR::WHITE, COLOR::BLACK);					  getline(cin, input);	this->ServerIP = input.c_str();
	setColor(COLOR::DARK_GRAY, COLOR::BLACK);  gotoXY(1, 11); cout << "ServerIP: ";


	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK); gotoXY(1, 12); cout << "Username: ";
	setColor(COLOR::WHITE, COLOR::BLACK);					  getline(cin, this->UserInfo.Username);
	setColor(COLOR::DARK_GRAY, COLOR::BLACK);  gotoXY(1, 12); cout << "Username: ";

	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK); gotoXY(1, 13); cout << "Password: ";
	setColor(COLOR::WHITE, COLOR::BLACK);	                  UserInfo.Password = hidePassword();
	setColor(COLOR::DARK_GRAY, COLOR::BLACK);  gotoXY(1, 13); cout << "Password: ";

	setColor(COLOR::DARK_GRAY, COLOR::BLACK);  gotoXY(0, 14); cout << "'============================'";
}

void Program::printStatus() {
	if ((selected != SELECTED::YES) && (selected != SELECTED::NO)) {
		gotoXY(4, 16);

		setColor(COLOR::BLACK, COLOR::LIGHT_GREEN);
		cout << "        ONLINE        ";

	}
	else {
		gotoXY(4, 16);

		setColor(COLOR::BLACK, COLOR::DARK_GRAY);
		cout << " DISCONECT ";

		if (selected == SELECTED::YES) {

			setColor(COLOR::BLACK, COLOR::LIGHT_RED);
			cout << " YES ";

			setColor(COLOR::LIGHT_BLUE, COLOR::BLACK);
			cout << "  NO  ";
		}

		if (selected == SELECTED::NO) {

			setColor(COLOR::LIGHT_RED, COLOR::BLACK);
			cout << " YES ";

			setColor(COLOR::BLACK, COLOR::LIGHT_BLUE);
			cout << "  NO  ";
		}

	}
	setColor(COLOR::WHITE, COLOR::BLACK);
}

void Program::printProgressBar(float percentage) {
	int val = (int)(percentage * 100);
	int width = (int)(percentage * 29);

	gotoXY(69, line_3);
	cout << "Progress ";

	gotoXY(78, line_3);
	cout << "[                             ]";
	
	setColor(COLOR::DARK_GRAY, COLOR::BLACK);
	gotoXY(79, line_3);
	for (int i = 0; i < width; i++) {
		cout << "|";
	}

	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK);
	string per = numCommas(val) + "%";
	gotoXY((int)((79 + 108 - per.length())/2), line_3);
	cout << per;

	setColor(COLOR::WHITE, COLOR::BLACK);

	if (abs(1 - percentage) < 0.0000001) {
		line_3++;
	}
}

string Program::shortenFileName(string filename) {
	if (filename.length() > 18) {
		string str1 = filename.substr(0, 9);
		string str2 = filename.substr(filename.length() - 6, 6);
		filename = str1 + "..." + str2;
	}

	return filename;
}