#pragma pack(push,1)


//typedef struct{
//	double obs_x[683];
//	double obs_y[683];
//	double x;
//	double y;
//	double theta;
//	double timestamp;
//	int num;
//
//}urg04lx, *urg04lx_p;

typedef struct{
	float	x;
	float	y;
	float	theta;
}arobotpose, *arobotposep;

typedef struct{
	float theta;
	float dist;
	int exitRobot;
}roboControl;

#pragma pack(pop)
