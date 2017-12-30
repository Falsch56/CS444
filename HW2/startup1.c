void clr_bss(void);
void init_devio(void);
void k_init(void);
void startupc(void);

void startupc(void){
	clr_bss(); 	//clear uninitialized data
	init_devio();   // tutor info
	k_init();       // start the kernel
}

