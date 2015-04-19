#define _CRT_SECURE_NO_DEPRECATE

#pragma comment(lib, "winmm.lib")

#include <Aria.h>
#include <ArActionGoto.h>

#include<math.h>
#include <ipc.h>
#include "urg.h"
#include "ipc_msg.h"
#include <process.h>
#include <iostream>
#include <stdio.h>
#include <windows.h>

using namespace std;

#define	MODULE_NAME	"winrobot"
//#define	PORT_SERIAL	"/dev/ttyUSB0"
#define	PORT_SERIAL	"COM4"


#define ON		0
#define OFF		1

#define INITIALIZED	0
#define INITIALIZING	1

bool isMove = false;


typedef struct mob_state {
	/* current statuses */
	int         robot;		/* INITIALIZING / INITIALIZED */
	int		motor;		/* ON / OFF */
	double	battery;	/* current voltage [V] */
	double	x;		/* current position [mm] */
	double	y;		/* current position [mm] */
	double	th;		/* current head angle [deg] */
	/* desired values */
	double	vel;		/* desired velocity [mm/sec] */
	double	head;		/* desired head angle [deg] */
} mob_state;


#define	MAX_SONAR	16

typedef struct sonar {
	int		num;		/* number of sonar sensors */
	double	x[MAX_SONAR];
	double	y[MAX_SONAR];
	double	th[MAX_SONAR];
	double	dist[MAX_SONAR];
} sonar;
mob_state	g_mob;
sonar		g_sonar;
JOYINFOEX JoyInfoEx;

ArRobot arrobot; //最常用类，这里用它连接机器人和传感器，取得自身数据，设置自身参数
ArPose g_arpose; //暂存自身位置
arobotpose g_arobotpose;//ipc传送自身位置
roboControl g_robotControl;//ipc接受控制行为的数据

// 3个线程
static unsigned __stdcall ipclistenThread(void *);
static unsigned __stdcall robotControlThread(void *);
static unsigned __stdcall ipcpublishThread(void *);
/*
static unsigned __stdcall ipclistenThread(LPVOID lpx);
static unsigned __stdcall robotControlThread(LPVOID lpx);
static unsigned __stdcall ipcpublishThread(LPVOID lpx);
*/
static void	ipc_init(void);
static void	ipc_close(void);
void robotControlHandler(MSG_INSTANCE ref, void *data, void *dummy);

HANDLE g_hMutexRobopo, g_hMutexRobocon;

int main(int argc, char *argv[])
{

	g_mob.robot = INITIALIZING;
	g_mob.motor = OFF;
	g_mob.battery = 0.0;
	g_mob.head = 0.0;
	g_mob.vel = 0.0;
	g_mob.x = 0.0;
	g_mob.y = 0.0;
	g_mob.th = 0.0;

	JoyInfoEx.dwSize = sizeof(JOYINFOEX);
	JoyInfoEx.dwFlags = JOY_RETURNALL;

	for (unsigned int i = 0; i<joyGetNumDevs(); i++){       //
		if (JOYERR_NOERROR == joyGetPosEx(i, &JoyInfoEx))
			printf("gamepad No.%d　connected!\n", i);
	}
	ipc_init();

	g_hMutexRobopo = CreateMutex(NULL, FALSE, "ROBOPOMUTEX");
	if (g_hMutexRobopo == NULL) {
		printf("CreateMutex(): Error\n");
	}
	g_hMutexRobocon = CreateMutex(NULL, FALSE, "ROBOCONMUTEX");
	if (g_hMutexRobocon == NULL) {
		printf("CreateMutex(): Error\n");
	}

	//创建多个线程
	HANDLE   ipclis, ipcpub, robotcon;
	unsigned  uiThread1ID, uiThread2ID, uiThread3ID;

	ipclis = (HANDLE)_beginthreadex(NULL,       // security  
		0,            // stack size  
		ipclistenThread,
		NULL,           // arg list  
		0,
		&uiThread1ID);
	if (ipclis == 0)
		cout
		<< "Failed to create ipclisten thread " << endl;

	robotcon = (HANDLE)_beginthreadex(NULL,       // security  
		0,            // stack size  
		robotControlThread,
		NULL,           // arg list  
		0,
		&uiThread2ID);
	if (robotcon == 0)
		cout << "Failed to create robotControl thread " << endl;

	ipcpub = (HANDLE)_beginthreadex(NULL,       // security  
		0,            // stack size  
		ipcpublishThread,
		NULL,           // arg list  
		0,
		&uiThread3ID);
	if (ipcpub == 0)
	    cout << "Failed to create ipcpublish thread " << endl;

	printf("ok01\n");
	while (1) {
		Sleep(100);
	}

	/* Close IPC */
	ipc_close();


	return 0;
}

static void ipc_init(void)
{
	/* Connect to the central server */
	if (IPC_connect(MODULE_NAME) != IPC_OK) {
		fprintf(stderr, "IPC_connect: ERROR!!\n");
		exit(-1);
	}

	IPC_defineMsg(ARPOSE_MSG, IPC_VARIABLE_LENGTH, ARPOSE_MSG_FMT);//ｷ｢ﾋﾍｻ憘ﾋﾎｻﾖﾃ
	IPC_defineMsg(ROBOCONTROL_MSG, IPC_VARIABLE_LENGTH, ROBOCONTROL_MSG_FMT);//ｽﾓﾊﾜlenﾎｻﾖﾃ
	IPC_subscribeData(ROBOCONTROL_MSG, robotControlHandler, NULL);
}


static void ipc_close(void)
{
	printf("Close IPC connection\n");
	IPC_disconnect();
}



void robotControlHandler(MSG_INSTANCE ref, void *data, void *dummy)   //取得控制行为的数据
{
	WaitForSingleObject(g_hMutexRobocon, INFINITE);
	g_robotControl = *(roboControl *)data;
	ReleaseMutex(g_hMutexRobocon);
}


//static unsigned __stdcall ipclistenThread(LPVOID lpx)
static unsigned __stdcall ipclistenThread(void *)
{
	HANDLE hM;
	static long i;

	//hM = *(HANDLE*)lpx;

	fprintf(stderr, "Start ipc_listen\n");
	i = 0;
	while (1) {
		//if (i % 20 == 0) fprintf(stderr, "IPC_listen: true (%ld)\n", i);
		printf("ipclisten\n");
		//pthread_mutex_lock(&g_mutex_ipc);
		IPC_listenWait(20);	/* 20 is stable */
		//pthread_mutex_unlock(&g_mutex_ipc);	
		i++;
		//fprintf(stderr, "ipc_listen: i = %ld\n", i);
		Sleep(50);
	}

	fprintf(stderr, "Stop ipc_listen\n");
}

//static unsigned __stdcall ipcpublishThread(LPVOID lpx)
static unsigned __stdcall ipcpublishThread(void *)
{
	HANDLE hM;
	static arobotpose	old_arobotpose;

	//hM = *(HANDLE*)lpx;

	fprintf(stderr, "Start ipc_publish\n");
	while (1) {
		WaitForSingleObject(g_hMutexRobopo, INFINITE);
		printf("ipcpublish\n");
		if (fabs(old_arobotpose.x - g_arobotpose.x)>1.0 ||   //当机器人位置变化超过阈值时打印并传送自身位置
			fabs(old_arobotpose.y - g_arobotpose.y)>1.0 ||
			fabs(old_arobotpose.theta - g_arobotpose.theta)>0.1) {
			printf("arobot_x = %lf,arobot_y = %lf,arobot_th = %lf\n",
				g_arobotpose.x, g_arobotpose.y, g_arobotpose.theta);
			IPC_publishData(ARPOSE_MSG, &g_arobotpose);
			old_arobotpose = g_arobotpose;

		}
		ReleaseMutex(g_hMutexRobopo);

		Sleep(100);
	}
}

//static unsigned __stdcall robotControlThread(LPVOID lpx)
static unsigned __stdcall robotControlThread(void *)
{
	HANDLE hM;
	char key, key_old;
	int	 i, argc = 3;
	char *argv[3];
	char hoge[3][1000] = { "./robot_control", "-robotPort", PORT_SERIAL };
	int	 num_sonar;
	ArSonarDevice dev_sonar;
	ArSensorReading *sonar;

	//hM = *(HANDLE*)lpx;

	ArActionDesired myDesired;
	myDesired.setMaxRotVel(2);

	for (i = 0; i < argc; i++) argv[i] = hoge[i];

	fprintf(stderr, "Start robot_control\n");

	/* ｳｼｻｯ Aria library */
	Aria::init();
	ArArgumentParser parser(&argc, (char**)argv);
	parser.loadDefaultArguments();

	//ﾁｬｽﾓ
	ArRobotConnector robotConnector(&parser, &arrobot);

	if (!robotConnector.connectRobot()) {
		// Error connecting:
		// if the user gave the -help argumentp,
		// then just print out what happened,
		// and continue so options can be displayed later.
		if (!parser.checkHelpAndWarnUnparsed()) {
			ArLog::log(ArLog::Terse, "Could not connect to robot, will not have parameter file so options displayed later may not include everything");
		}
		else {	// otherwise abort
			ArLog::log(ArLog::Terse, "Error, could not connect to robot.");
			Aria::logOptions();
			Aria::exit(1);
		}
	}

	arrobot.runAsync(true);

	arrobot.addRangeDevice(&dev_sonar);

	ArUtil::sleep(1000);
	// Motor ON
	arrobot.comInt(ArCommands::ENABLE, 1);
	arrobot.comInt(ArCommands::SONAR, 1);
	g_mob.motor = ON;

	g_sonar.num = arrobot.getNumSonar();


	// Goto action at lower priority
	ArActionGoto gotoPoseAction("goto");
	arrobot.addAction(&gotoPoseAction, 50);

	//// Stop action at lower priority, so the robot stops if it has no goal
	//ArActionStop stopAction("stop");
	//arrobot.addAction(&stopAction, 40);

	// turn on the motors, turn off amigobot sounds
	arrobot.enableMotors();
	arrobot.comInt(ArCommands::SOUNDTOG, 0);

	while (1){
		printf("ipcrobot\n");

		WaitForSingleObject(g_hMutexRobocon, INFINITE);
		if (g_robotControl.exitRobot == 1)break;
		ReleaseMutex(g_hMutexRobocon);

		//取得机器人位置，传送和打印在其他线程中
		WaitForSingleObject(g_hMutexRobopo, INFINITE);
		if (isMove = true)
		{
			
			g_arpose = arrobot.getPose();
			g_arobotpose.x = g_arpose.getX();
			g_arobotpose.y = g_arpose.getY();
			g_arobotpose.theta = -g_arpose.getTh();
		}

		if (g_arobotpose.theta < 0)g_arobotpose.theta += 360;
		ReleaseMutex(g_hMutexRobopo);

		//printf("robotcontrol-theta = %lf\n", g_robotControl.theta);
		JOYERR_NOERROR == joyGetPosEx(0, &JoyInfoEx);

		WaitForSingleObject(g_hMutexRobocon, INFINITE);

		cout << "g_robotControl.dist : " << g_robotControl.dist << "g_robotControl.theta : " << g_robotControl.theta << endl;
		//cout << "JoyInfoEx.dwXpos : " << JoyInfoEx.dwXpos << "&JoyInfoEx.dwYpos : " << JoyInfoEx.dwYpos << endl;
		if (JoyInfoEx.dwXpos == 32767 && JoyInfoEx.dwYpos < 10000){
			arrobot.setVel2(40, 40);
			cout << "1" << endl;
		}
		else if (JoyInfoEx.dwXpos < 10000 && JoyInfoEx.dwYpos < 50000 && JoyInfoEx.dwYpos>10000){
			arrobot.setVel2(-40, 40);
			cout << "2" << endl;
		}
		else if (JoyInfoEx.dwXpos > 50000 && JoyInfoEx.dwYpos < 50000 && JoyInfoEx.dwYpos>10000){
			arrobot.setVel2(40, -40);
			cout << "3" << endl;
		}
		else if (JoyInfoEx.dwXpos < 50000 && JoyInfoEx.dwYpos > 50000){
			arrobot.setVel2(-40, -40);
			cout << "4" << endl;
		}
		else if (g_robotControl.dist<80 && fabs(g_robotControl.theta)>20)
		{
			arrobot.setVel2(g_robotControl.theta, -g_robotControl.theta);
			isMove = true;
		}
		else if (g_robotControl.dist > 80)
		{
			arrobot.setVel2(0.4*g_robotControl.dist + g_robotControl.theta, 0.4*g_robotControl.dist - g_robotControl.theta);
			isMove = true;
		}
		else {
			arrobot.setVel2(0, 0);
			arrobot.moveTo(g_arpose);
			isMove = false;
//			Sleep(50);
			if (JoyInfoEx.dwButtons == 2)break;
		}
		ReleaseMutex(g_hMutexRobocon);
		Sleep(100);
	}
	arrobot.setVel2(0, 0);
	arrobot.disableMotors();
	g_mob.motor = OFF;

	arrobot.stopRunning();
	arrobot.waitForRunExit();

	Aria::exit(0);
	fprintf(stderr, "Stop robot_control\n");
	return 0;
}
