#ifndef PROGRAM_H_
#define PROGRAM_H_


// forward declaration
struct App;

struct Program;
typedef struct Program Program;

extern Program program;

extern int Program_main(void);
extern void Program_exit(void);


struct Program
{
   //struct App* app; // became unnecessary
   int registerRes;
};


#endif /*PROGRAM_H_*/
