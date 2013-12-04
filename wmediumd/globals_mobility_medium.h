
#ifndef SHAREFILE_INCLUDED
#define SHAREFILE_INCLUDED
#ifdef  MAIN_FILE

int debug;
struct mobility_medium_cfg mob_med_cfg;
int mobility;
unsigned long int start_execution_timestamp;
int last_def_position;
int dcurrent;

#else

extern debug;
extern struct mobility_medium_cfg mob_med_cfg;
extern char mode;
extern unsigned long int start_execution_timestamp;
extern int last_def_position;
extern int dcurrent;

#endif
#endif

