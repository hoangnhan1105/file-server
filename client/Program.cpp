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
	this->initWinsock();
	
	this->ExitFlag = false;

	/* LOG */
	fstream f(DATABASE_PATH + "\\" + LOG_FILE, std::fstream::out | std::fstream::app);
	time_t now = time(0);
	tm* ltm = localtime(&now);
	f << "================ Date: " << ltm->tm_mday << " - " << 1 + ltm->tm_mon << " - " << (1900 + ltm->tm_year) << " ================" << endl;
	f.close();
}

Program::~Program()
{
	// Release resources.
	WSACleanup();
}

void Program::run()
{ 
	this->homeScreen();
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

bool Program::initConnectSocket()
{
	struct addrinfo* result = nullptr;	// A pointer to a linked list of one or more addrinfo structures that contains response information about the host.
	struct addrinfo hints;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(this->ServerIP, this->SERVER_PORT, &hints, &result);	// Update 'result' with port, IP address,...

	// Wrong Server IP
	if (iResult != 0) {
		this->LastError = "getaddrinfo() failed: " + std::to_string(iResult);
		this->printLastError();

		// Log
		string gui_2 = string("Connection fail.");
		string gui_1 = string("Server IP: ") + string(this->ServerIP);
		printLog(gui_1, gui_2, gui_2);

		return false;
	}

	this->UserInfo.ConnectSocket = INVALID_SOCKET;
	
	for (auto ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		this->UserInfo.ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (this->UserInfo.ConnectSocket == INVALID_SOCKET) {
			this->LastError = "socket() failed with error: " + std::to_string(WSAGetLastError());
			this->printLastError();
			return false;
		}

		iResult = connect(this->UserInfo.ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

		if (iResult == SOCKET_ERROR) {
			closesocket(this->UserInfo.ConnectSocket);
			this->UserInfo.ConnectSocket = INVALID_SOCKET;
			continue;
		}

		// Log
		string gui_2 = string("Connection success.");
		string gui_1 = string("Server IP: ") + string(this->ServerIP);
		printLog(gui_1, gui_2, gui_2);

		break;
	}

	if (this->UserInfo.ConnectSocket == INVALID_SOCKET) {
		this->LastError = "Error at socket(): " + std::to_string(WSAGetLastError());
		this->printLastError();
		freeaddrinfo(result);

		// Log
		string gui_2 = string("Connection fail.");
		string gui_1 = string("Server IP: ") + string(this->ServerIP);
		printLog(gui_1, gui_2, gui_2);

		return false;
	}

	freeaddrinfo(result);

	return true;
}

void Program::receiveMsg()
{
	/* Message structure: FLAG (uint8_t) | MSGLEN (uint64_t) | MSG (char*) */

	int crashFlag;

	RcvMsgFlag flag;
	uint64_t msgLen;
	char* msg;

	while (true) {
		crashFlag = this->receiveData((char*)&flag, sizeof(flag));
		if (crashFlag == -1) {
			// Client close rcv.
			closesocket(this->UserInfo.ConnectSocket);

			// Log
			std::string log = "<Server> logged out.";
			this->printLog(log, log);

			this->ExitFlag = true;

			break;
		}

		this->receiveData((char*)&msgLen, sizeof(msgLen));
		msg = new char[msgLen + 1];

		this->receiveData(msg, msgLen);
		msg[msgLen] = '\0';

		switch (flag)
		{
		case RcvMsgFlag::REGISTER_FAIL: {
			// Log
			string gui_1 = "Register failed. Username already exists.";
			printLog(gui_1, gui_1);

			// Register_Fail -> selected Login/Register
			this->printClient();
			this->navigateClient();

			break;
		}
		case RcvMsgFlag::REGISTER_SUCCESS: {
			// Log
			string gui_1 = "<" + UserInfo.Username + ">" + " registered success.";
			printLog(gui_1, gui_1);

			// After Register -> Login
			selected = SELECTED::LOGIN;
			this->buttonClient();
			this->printClient();
			this->loginClient();
			this->tryLogin();

			break;
		}
		case RcvMsgFlag::LOGIN_FAIL_USERNAME: {
			// Log
			string gui_1 = "Login failed. Username doesn't exist";
			printLog(gui_1, gui_1);

			// Login_Fail -> selected Login/Register
			this->printClient();
			this->navigateClient();

			break;
		}
		case RcvMsgFlag::LOGIN_FAIL_PASSWORD: {
			// Log
			string gui_1 = "Login failed. Wrong password";
			printLog(gui_1, gui_1);

			// Login_Fail -> selected Login/Register
			this->printClient();
			this->navigateClient();

			break;
		}
		case RcvMsgFlag::LOGIN_FAIL_ONLINE: {
			// Log
			string gui_1 = "Login failed.";
			string gui_2 = "This account is currently online.";
			printLog(gui_1, gui_2, gui_2);

			// Login_Fail -> selected Login/Register
			this->printClient();
			this->navigateClient();

			break;
		}
		case RcvMsgFlag::LOGIN_SUCCESS: {
			// Log
			string gui_1 = "<" + UserInfo.Username + ">" + " logged in success.";
			printLog(gui_1, gui_1);

			// Gui
			selected = SELECTED::UPLOAD;
			this->printStatus();

			break;
		}
		case RcvMsgFlag::UPLOAD_FILE_FAIL: {
			// Log
			std::string gui1 = "Upload file failed.";
			std::string gui2 = "This file's name already exists.";
			std::string log = gui1 + " " + gui2;
			printLog(gui1, gui2, log);
      
			break;
		}
		case RcvMsgFlag::UPLOAD_FILE_SUCCESS: {
			/*std::thread uploadFileThread(&Program::uploadFile, this, this->LastUploadedFilePath);
			uploadFileThread.detach();*/
			this->uploadFile(this->LastUploadedFilePath);
			break;
		}
		case RcvMsgFlag::DOWNLOAD_FILE_SUCCESS: {
			this->receiveADownloadFileReply(this->dirDownFolder);
			break;
		}
		case RcvMsgFlag::NEW_USER_LOGIN: {
			this->writeLogNewLogin();
			break;
		}
		case RcvMsgFlag::NEW_FILE_LIST: {
			std::string initFileContent(msg);
			this->initSharedFileList(initFileContent);
			break;
		}
		case RcvMsgFlag::NEW_FILE: {
			std::string newFileContent(msg);
			this->updateSharedFileList(newFileContent);
			break;
		}
		case RcvMsgFlag::LOGOUT_CLIENT: {
			int temp;
			int shutdownCheck = receiveData((char*)&temp, sizeof(temp));
			if (shutdownCheck == 0) {
				this->receiveALogoutReply();
				this->ExitFlag = true;
			}

			break;
		}
		case RcvMsgFlag::LOGOUT_SERVER: {
			int temp;
			int shutdownCheck = receiveData((char*)&temp, sizeof(temp));
			if (shutdownCheck == 0) {
				this->sendALogoutReply();
				this->ExitFlag = true;
			}

			break;
		}
		case RcvMsgFlag::CLIENT_LOGOUT_NOTIF: {
			std::string username(msg);
			std::string log = "<" + username + "> logged out.";
			this->printLog(log, log);

			break;
		}
		default:
			break;
		}

		delete[] msg;
		msg = nullptr;

		if (this->ExitFlag) {
			break;
		}
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

void Program::tryRegister() {
	size_t usernameLen;
	size_t passwordLen;

	sendMsg(SendMsgFlag::REGISTER, "\0", 1);

	usernameLen = UserInfo.Username.length();
	sendData((char*)&usernameLen, sizeof(usernameLen));
	sendData(UserInfo.Username.c_str(), UserInfo.Username.length() + 1);
	passwordLen = UserInfo.Password.length();
	sendData((char*)&passwordLen, sizeof(passwordLen));
	sendData(UserInfo.Password.c_str(), UserInfo.Password.length() + 1);
}

void Program::tryLogin() {
	size_t usernameLen;
	size_t passwordLen;

	sendMsg(SendMsgFlag::LOGIN, "\0", 1);

	usernameLen = UserInfo.Username.length();
	sendData((char*)&usernameLen, sizeof(usernameLen));
	sendData(UserInfo.Username.c_str(), UserInfo.Username.length() + 1);
	passwordLen = UserInfo.Password.length();
	sendData((char*)&passwordLen, sizeof(passwordLen));
	sendData(UserInfo.Password.c_str(), UserInfo.Password.length() + 1);
}

void Program::writeLogNewLogin() {
	char* username;
	size_t usernameLen;

	receiveData((char*)&usernameLen, sizeof(usernameLen));
	username = new char[usernameLen + 1];
	receiveData(username, usernameLen + 1);

	string usernameString(username);

	// Log
	string gui_1 = "<" + usernameString + "> logged in.";	// PRINT LOG
	printLog(gui_1, gui_1);
}

void Program::sendADownloadFileRequest(uint64_t const& fileIndex)
{
	/* Message structure: FLAG (uint8_t) | MSGLEN (uint64_t) | MSG (string) */

	// Send a request to the Server first.
	SendMsgFlag flag = SendMsgFlag::DOWNLOAD_FILE;
	std::string msg = std::to_string(fileIndex);
	uint64_t msgLen = msg.length();

	// Log
	string gui_1 = string("Request to download: ");
	string gui_2 = shortenFileName(FileList[line_2].fileName) + string(" (") + FileList[line_2].fileSize + string(").");
	string log = FileList[line_2].fileName + string(" (") + FileList[line_2].fileSize + string(").");
	printLog(gui_1, gui_2, log);

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

		ShowConsoleCursor(false);

		// Receive file's size
		this->receiveData((char*)&fileSize, sizeof(fileSize));

		// Log
		string gui_1 = string("Start downloading: ");
		string gui_2 = FileList[line_2].fileName + string(" (") + FileList[line_2].fileSize + string(").");
		printLog(gui_1, gui_2, gui_2);

		printProgressBar(0);// Start 0%
		
		// Receive file's data
		for (uint64_t i = 0; i < fileSize / this->BUFFER_LEN; ++i) {
			this->receiveData(buffer, this->BUFFER_LEN);
			fout.write(buffer, this->BUFFER_LEN);

			//Progress
			if (i % 500 == 0)
				printProgressBar((i + 1) * this->BUFFER_LEN * 1.0 / fileSize);
		}

		this->receiveData(buffer, fileSize % this->BUFFER_LEN);
		fout.write(buffer, fileSize % this->BUFFER_LEN);

		printProgressBar(1); // Complete 100%

		// Log
		gui_1 = string("Download succeess: ");
		gui_2 = FileList[line_2].fileName + string(" (") + FileList[line_2].fileSize + string(").");
		printLog(gui_1, gui_2, gui_2);

		ShowConsoleCursor(true);

		// Release resources
		delete[] buffer;
		fout.close();
	}
	else {
		this->LastError = "Unable to open file " + downloadedFilePath;
		this->printLastError();
	}
}

void Program::sendAnUploadFileRequest(std::string const& uploadedFilePath)
{
	this->LastUploadedFilePath = uploadedFilePath;

	std::string fileName = this->getFileNameFromPath(uploadedFilePath);

	// Send a request to the Server with the corresponding flag and the file's name.
	SendMsgFlag flag = SendMsgFlag::UPLOAD_FILE;
	const char* msg = fileName.c_str();
	uint64_t msgLen = fileName.length();

	this->sendMsg(flag, msg, msgLen);

	// Log
	string gui = string("Request to upload ") + shortenFileName(fileName) + string(".");
	string log = string("Request to upload ") + shortenFileName(fileName) + string(".");
	printLog(gui, log);

	// Waiting for a reply from the Server.
}

void Program::uploadFile(std::string const& uploadedFilePath)
{
	std::string fileName = this->getFileNameFromPath(uploadedFilePath);

	ifstream fin(uploadedFilePath, std::ios_base::binary);

	if (fin.is_open()) {
		int iResult;

		uint64_t fileSize;
		char* buffer = new char[this->BUFFER_LEN];

		// Get file's size.
		fin.seekg(0, std::ios_base::end);
		fileSize = fin.tellg();
		fin.seekg(0, std::ios_base::beg);

		// Add this file to the Database.
		File file;
		file.fileName = fileName;
		file.fileSize = this->shortenFileSize(fileSize);
		this->FileList.push_back(file);

		// Send file's size
		this->sendData((char*)&fileSize, sizeof(fileSize));

		// Log: start uploading
		std::string gui_1 = string("Start uploading: ");
		std::string gui_2 = fileName + string(" (") + shortenFileSize(fileSize) + string(").");
		printLog(gui_1, gui_2, gui_2);

		ShowConsoleCursor(false);

		printProgressBar(0); // Start 0%

		// Send file's data
		for (uint64_t i = 0; i < fileSize / this->BUFFER_LEN; ++i) {
			fin.read(buffer, this->BUFFER_LEN);
			this->sendData(buffer, this->BUFFER_LEN);

			// Progress
			if (i % 500 == 0)
				printProgressBar((i + 1) * this->BUFFER_LEN * 1.0 / fileSize);
		}
		fin.read(buffer, fileSize % this->BUFFER_LEN);
		this->sendData(buffer, fileSize % this->BUFFER_LEN);

		printProgressBar(1); // Complete 100%

		ShowConsoleCursor(true);

		// Log
		gui_1 = string("Upload succeess: ");
		gui_2 = fileName + string(" (") + shortenFileSize(fileSize) + string(").");
		printLog(gui_1, gui_2, gui_2);

		// Gui
		line_2 = FileList.size() - 1;
		printFile(shortenFileName(fileName), shortenFileSize(fileSize), false);
		line_2 = 0;


		// Release resources
		delete[] buffer;
		fin.close();
	}
	else {
		this->LastError = "Unable to open file " + uploadedFilePath;
		this->printLastError();
	}
}

void Program::updateSharedFileList(std::string const& newFileContent)
{
	std::string username;
	File newFile;

	int i = 0;
	int j = 0;

	for (i = 0; i < newFileContent.length(); ++i) {
		if (newFileContent[i] == '\n') {
			username = newFileContent.substr(0, i);
			break;
		}
	}

	for (j = newFileContent.length() - 1; j >= 0; --j) {
		if (newFileContent[j] == '\n') {
			newFile.fileSize = newFileContent.substr(j + 1, newFileContent.length() - j - 1);
			break;
		}
	}

	newFile.fileName = newFileContent.substr(i + 1, j - i - 1);

	this->FileList.push_back(newFile);

	// GUI
	line_2 = FileList.size() - 1;
	this->printFile(shortenFileName(newFile.fileName), newFile.fileSize, false);
	line_2 = 0;

	// ... Log
	string gui_1 = "<" + username + ">" + " uploaded file: ";
	string gui_2 = shortenFileName(newFile.fileName) + string(" (") + newFile.fileSize + string(").");
	string log = newFile.fileName + string(" (") + newFile.fileSize + string(").");
	printLog(gui_1, gui_2, log);

}

void Program::initSharedFileList(std::string const& initFileContent) {
	File initFile;

	for (size_t i = 0; i < initFileContent.length(); ++i) {
		if (initFileContent[i] == '\n') {
			initFile.fileName = initFileContent.substr(0, i);
			initFile.fileSize = initFileContent.substr(i + 1, initFileContent.length() - i - 1);
			break;
		}
	}

	this->FileList.push_back(initFile);
	

	// GUI
	line_2 = FileList.size() - 1;
	this->printFile(shortenFileName(initFile.fileName), initFile.fileSize, false);
	line_2 = 0;
}

void Program::sendALogoutRequest()
{
	SendMsgFlag flag = SendMsgFlag::LOGOUT_CLIENT;
	std::string msg = "a";
	uint64_t msgLen = msg.length();

	this->sendMsg(flag, msg.c_str(), msgLen);

	// Client closes send.
	int iResult = shutdown(this->UserInfo.ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		this->LastError = "shutdown() failed: " + std::to_string(WSAGetLastError());
		this->printLastError();
	}

	// Log
	std::string log = "Request the Server to logout.";
	this->printLog(log, log);
}

void Program::receiveALogoutReply()
{
	// Client close rcv.
	closesocket(this->UserInfo.ConnectSocket);
}

void Program::sendALogoutReply()
{
	SendMsgFlag flag = SendMsgFlag::LOGOUT_SERVER;
	std::string msg = "a";
	uint64_t msgLen = msg.length();

	this->sendMsg(flag, msg.c_str(), msgLen);

	// Client close send.
	int iResult = shutdown(this->UserInfo.ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		this->LastError = "shutdown() failed: " + std::to_string(WSAGetLastError());
		this->printLastError();
	}

	// Client close rcv.
	closesocket(this->UserInfo.ConnectSocket);

	// Log
	std::string log = "<Server> logged out.";
	this->printLog(log, log);
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
	//cout << this->LastError << "\n";
}


void Program::homeScreen() {
	do {
		this->printTitle();
		this->printMode();
		this->printIP();
		this->printClient();
	} while (!this->navigateClient());

	while (selected != SELECTED::UPLOAD);


	for (int i = 0; i < FileList.size(); i++) {
		printFile(shortenFileName(FileList[i].fileName), FileList[i].fileSize, false);
		line_2++;
	}
	line_2 = 0;
	
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
						printFile(shortenFileName(FileList[line_2].fileName), FileList[line_2].fileSize, false); // reset the previous line back to normal
						line_2 = 0;									  // reset line back to the top of FileList
						if (selected == SELECTED::DOWNLOAD) {
							printFile(shortenFileName(FileList[0].fileName), FileList[0].fileSize, true);
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
						printFile(shortenFileName(FileList[line_2].fileName), FileList[line_2].fileSize, false); // reset the previous line back to normal
						line_2--;
						line_2 += (line_2 < 0) ? FileList.size() : 0;
						printFile(shortenFileName(FileList[line_2].fileName), FileList[line_2].fileSize, true);
					}
				}
			}

			// ============= DOWN =============
			if (GetKeyState(VK_DOWN) & 0x8000) {
				if (selected == SELECTED::DOWNLOAD) {
					if (FileList.size() > 0) {
						printFile(shortenFileName(FileList[line_2].fileName), FileList[line_2].fileSize, false); // reset the previous line back to normal
						line_2++;
						line_2 %= FileList.size();
						printFile(shortenFileName(FileList[line_2].fileName), FileList[line_2].fileSize, true);
					}
				}
			}

			// ============= ENTER =============
			if (GetKeyState(VK_RETURN) & 0x8000) {

				if (selected == SELECTED::UPLOAD) {
					std::string uploadedFilePath = this->enterPath();

					if (uploadedFilePath.length() > 2 && uploadedFilePath.front() == '"' && uploadedFilePath.back() == '"') {
						uploadedFilePath.erase(uploadedFilePath.begin());
						uploadedFilePath.erase(uploadedFilePath.begin() + uploadedFilePath.length() - 1);
					}

					if (isFilePathExist(uploadedFilePath)) {
						this->sendAnUploadFileRequest(uploadedFilePath);
					}
					else {
						// Log
						string content = "Error file's path. Fail to upload.";
						printLog(content, content);
					}
					
				}

				if (selected == SELECTED::DOWNLOAD) {
					if (FileList.size() > 0) {
						this->dirDownFolder = this->enterPath();

						if (dirExists(dirDownFolder)) {
							// Log
							string gui_1 = "Download folder path: ";
							string gui_2 = dirDownFolder + "\\";
							printLog(gui_1, gui_2, gui_2);
							dirDownFolder = this->dirDownFolder + string("\\") + this->FileList[this->line_2].fileName;
							this->sendADownloadFileRequest(this->line_2);
						}
						else {
							if (this->dirDownFolder == "") {
								// Log
								string gui_1 = "Download folder path: Default";
								printLog(gui_1, gui_1);
								this->dirDownFolder = this->DATABASE_PATH + "\\" + this->DOWNLOAD_FOLDER + "\\" + this->FileList[this->line_2].fileName;	// default path
								this->sendADownloadFileRequest(this->line_2);
							}
							else {
								// Log
								string gui_1 = "Download folder path: ";
								string gui_2 = dirDownFolder + "is invalid.";
								printLog(gui_1, gui_2, gui_2);
							}
						}
					}
				}

				if (selected == SELECTED::YES) {	// Logout here
					esc = true;
					this->sendALogoutRequest();
					while (!this->ExitFlag);
				}

				if (selected == SELECTED::NO) {
					selected = SELECTED::UPLOAD;
					printStatus();
				}
			}

			// ============= ESC =============
			if (GetKeyState(VK_ESCAPE) & 0x8000) {
				if (FileList.size() > 0) {
					printFile(shortenFileName(FileList[line_2].fileName), FileList[line_2].fileSize, false); // reset the previous line back to normal
					line_2 = 0;									  // reset line back to the top of FileList
				}
				selected = SELECTED::UPLOAD;
				this->printMode();

				selected = SELECTED::NO;
				this->printStatus();
			}
		}

		if (esc || this->ExitFlag) break;
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
	cout << " " << shortenFileName(name); printSpace(27 - name.length() - size.length()); cout << size << " ";
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

	cout << timeline << std::flush;
	f << timeline;

	// Content
	setColor(COLOR::WHITE, COLOR::BLACK);
	cout << gui << std::flush;
	f << log << endl;

	line_3++;
	f.close();
}

void Program::printLog(string gui_1, string gui_2, string log) {	
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

	cout << timeline << std::flush;
	f << timeline;

	// Content
	setColor(COLOR::WHITE, COLOR::BLACK);
	cout << gui_1 << std::flush; line_3++;
	gotoXY(69, line_3);
	cout << gui_2 << std::flush;

	f << gui_1 << endl << log << endl;

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

void Program::printIP() {
	setColor(COLOR::DARK_GRAY, COLOR::BLACK);
	gotoXY(0, 11); cout << "|                            |";

	setColor(COLOR::DARK_GRAY, COLOR::BLACK);
	gotoXY(1, 11); cout << "ServerIP:";
}

void Program::printClient() {
	gotoXY(0, 9); 
	setColor(COLOR::LIGHT_CYAN, COLOR::BLACK);
	printTextAtMid("[CLIENT]", 0, 30);

	buttonClient();

	setColor(COLOR::DARK_GRAY, COLOR::BLACK);
	gotoXY(0, 12); cout << "|                            |";
	gotoXY(0, 13); cout << "|                            |";
	gotoXY(0, 14); cout << "'============================'";

	setColor(COLOR::DARK_GRAY, COLOR::BLACK);
	gotoXY(1, 12); cout << "Username:";
	gotoXY(1, 13); cout << "Password:";
}

bool Program::navigateClient() {
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
				if (this->ServerIP == NULL || this->ServerIP[0] == 0) {
					if (!this->inputIP()) { // make sure ip right
						this->ServerIP = NULL;
						return false;
					}
				}
				
				if (selected == SELECTED::REGISTER) {
					this->loginClient();

					this->tryRegister(); // make sure register success
					
				}
				else {
					this->loginClient();

					this->tryLogin(); // make sure login success
				}

				break;
			}
		}

	}

	return true;
}

bool Program::inputIP() {
	std::string input;

	do {
		setColor(COLOR::LIGHT_CYAN, COLOR::BLACK); gotoXY(1, 11); cout << "ServerIP: ";
		setColor(COLOR::WHITE, COLOR::BLACK);					  getline(cin, input);
		setColor(COLOR::DARK_GRAY, COLOR::BLACK);  gotoXY(1, 11); cout << "ServerIP: ";
	} while (input == "");

	this->ServerIP = input.c_str();

	if (this->initConnectSocket()) {
		std::thread rcvMsgThread(&Program::receiveMsg, this);
		rcvMsgThread.detach();

		return true;
	}

	return false;
}

void Program::loginClient() {
	do {
		setColor(COLOR::LIGHT_CYAN, COLOR::BLACK); gotoXY(1, 12); cout << "Username: ";
		setColor(COLOR::WHITE, COLOR::BLACK);					  getline(cin, this->UserInfo.Username);
		setColor(COLOR::DARK_GRAY, COLOR::BLACK);  gotoXY(1, 12); cout << "Username: ";
	} while (this->UserInfo.Username == "");

	do {
		setColor(COLOR::LIGHT_CYAN, COLOR::BLACK); gotoXY(1, 13); cout << "Password: ";
		setColor(COLOR::WHITE, COLOR::BLACK);	                  UserInfo.Password = hidePassword();
		setColor(COLOR::DARK_GRAY, COLOR::BLACK);  gotoXY(1, 13); cout << "Password: ";
	} while (this->UserInfo.Password == "");

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

string Program::shortenFileSize(unsigned long size) {
	uint8_t d = 0;

	while (size > 1000) {
		size /= 1000;
		d++;
	}

	string parameter[] = { " B", "KB", "MB", "GB" };

	return numCommas(size) + " " + parameter[d];
}

bool Program::isFilePathExist(const std::string& name) {
	ifstream f(name.c_str());
	return f.good();
}

bool Program::dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}