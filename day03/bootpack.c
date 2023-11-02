/* 告诉C编译器，有一个函数在其他的文件里 */
void io_hlt(void);


void HariMain(void){
fin:
	io_hlt(); /* naskfunc.nas里的_io_hlt函数 */
	goto fin;
}
