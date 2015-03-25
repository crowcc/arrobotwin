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
	//double w_y;
	//int    needTurn;//true:1 false:0
	//int    needMov;//true:1 false:0
}roboControl;
