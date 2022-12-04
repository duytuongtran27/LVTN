#include "IRRecv.h"

void IRRecvRXCreateTask(void){
    IRDriverRXCreateTask();
}
void IRRecvRXDeleteTask(void){
    IRDriverRXDeleteTask();
}
void IRRecvRAWCreateTask(void){
    IRDriverRAWCreateTask();
}
void IRRecvRAWDeleteTask(void){
    IRDriverRAWDeleteTask();
}
void IRRecvSetStartCondition(uint32_t Mark, uint32_t Space, uint32_t Tolerance){
    IRDriverSetStartCondition(Mark, Space, Tolerance);
}
char* IRRecvGetCaptureStream(void){
    return IRDriverGetCaptureStream();
}