#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>


#define scr_width 320
#define scr_height 240
#define scr_BPP 2 //16 bits per pixel(in bytes) 

#define BLOCK_SIZE 5 // size of block is 5x5 pixels

#define BLOCK_SIZE_X (scr_width/BLOCK_SIZE)
#define BLOCK_SIZE_Y (scr_height/BLOCK_SIZE)
#define MAX_LENGHT 10


struct fb_copyarea screen;
int fbfd = 0, GPDD = 0; //GAMEPAD device driver

uint16_t *framebuffer;
typedef union {
		struct{ 
			uint16_t B : 11;
			uint16_t G : 11;
			uint16_t R : 11;
		};
		uint16_t toint;
} Color;

void draw_pixel(uint16_t x, uint16_t y, Color color){
	if(x < scr_width && x >= 0 && y < scr_height && y >=0){
		framebuffer [ x+ y*scr_width ] = color.toint;
	}else {
		printf("invalid pixel pos: (%d,%d), max %d x %d", x,y,(scr_width -1), (scr_height -1));
		}

}

int calc_devide(int a, int b)
{
	return a/b;
}

enum direction{
	NONE 	=  0,
	UP 	 	=  1,
	DOWN 	= -1,
	RIGHT 	=  2,
	LEFT 	= -2
};

void flush_fb(void){
	ioctl(fbfd, 0x4680, &screen);
}


Color color(uint8_t r, uint8_t g, uint8_t b) {
	Color col;
	if(r >= 0 && r < 32 && g >= 0 && g < 64 && b >= 0 && b < 32){
		col.R = r;
		col.G = g;
		col.B = b;
	} else {
		col.R = 0;
		col.G = 0;
		col.B = 0;
		printf("Invalid color value supplied: (R: %d, G: %d, B: %d), max (31, 63, 31)", r, g, b);
	}
	return col;
}

typedef union{
		struct{
			uint8_t x;
			uint8_t y;
		};
		uint16_t coordinates;
}coordinate;

coordinate Coordinate(uint8_t x, uint8_t y) {
	coordinate coord;
	if(x >= 0 && x<BLOCK_SIZE_X && y >= 0 && y<BLOCK_SIZE_Y){
		coord.x = x;
		coord.y = y;
	}
	else
	{
		coord.x = 0;
		coord.y = 0;
		printf("invalid coordinat, your value x %d, y %d, max x: %d, max y: %d",x,y,BLOCK_SIZE_X, BLOCK_SIZE_Y);
	}
	return coord;
}

typedef struct {
	uint8_t lenght;
	coordinate pos[MAX_LENGTH];
	Color color;
	enum direction dir;
} snake;


typedef struct {
	coordinate pos;
	Color color;
} Item;

Item Fruit;
snake Snake;



void draw_block(uint8_t x, uint8_t y, Color color)
{
	
	for(uint8_t i = 0; i < BLOCK_SIZE; i++)
	{
		for(uint8_t j = 0; j < BLOCK_SIZE; j++)
		{	
			draw_pixel(x*BLOCK_SIZE + 1, y*BLOCK_SIZE +j, color);
		}
	}
}

void clear_screen()
{
	memset(framebuffer, 0, scr_width*scr_width*scr_bpp);
	flush_fb();
}

int crash(coordinate point)
{
	if(point.x <0 || point.x >= BLOCK_SIZE_X || point.y < 0 || point.y >=BLOCK_SIZE_Y)
	{
		return 1; // we have a crash!
	}
	for(int i=0; i < Snake.lenght; i++)
	{
		if(snake.pos[i].coordinates == point.coordinates)
		{
			return 1; // we have a crash!
		}
	}
	return 0; // No crash!
}



void Move_snake(snake *Snake, enum direction dir)
{
	coordinate current_pos;
	if(dir == -Snake->dir || dir == NONE) return;
	Snake->dir = dir;
	current_pos = Snake->pos[0];
	switch(dir)
	{
		case UP:
			current_pos.y--;
			break;
		case DOWN:
			current_pos.y++;
			break;
		case LEFT:
			current_pos.x--;
			break;
		case RIGHT:
			current_pos.x++;
			break;
		default:
			return;
	}
	if(crash(current_pos))
	{
		printf("Snake Crashed! You lost");
		clear_screen();
		for(uint8_t i =0; i< Snake-> lenght; i++)
		{
			draw_block(Snake->pos[i].x, Snake->pos[i].y, Snake->color);
		}
		flush_fb();
		exit_clean();
	}
	
	for(uint8_t i = Snake->lenght; i >0; i--)
	{
		Snake->pos[i] = Snake->pos[i-1];
	}
	Snake->pos[0] = current_pos;
}





void place_fruit()
{
	srand(time(NULL));
	
	Fruit.pos.x = random() % BLOCK_SIZE_X;
	Fruit.pos.y = random() % BLOCK_SIZE_Y;
	if(crash(Fruit.pos))
	{
		printf("Position taken, placing fruit again!);
		place_fruit();
	}
	else
	{
		draw_block(Fruit.pos.x, Fruit.pos.y, Fruit.color);
	}
}





	
void button_push(int signal)
{
	int x = 0, y = 0;
	char button, input_buffer;
	if(read(GPDD,&input_buffer, 1) != -1){
		button = input_buffer & 0x0F; //Uses button 1-4
		switch(button){
			case 1:
				for(x = 0; x<= 50; x++){
					for(y = 0; y <= 10; y++){
						draw_pixel(x,y,color(x/10,y/10,(x+y)/10));
					}
				}
				break;
			case 2:
				for(x = 0; x<= 200; x++){
					for(y = 0; y <= 100; y++){
						draw_pixel(x,y,color(x/10,y/10,(x+y)/20));
					}
				}
				printf("button 2 pushed!");
				break;
			case 4:
				printf("button 3 pushed!");
				break;
			case 8:
				printf("button 4 pushed!");
				break;
			default:
				printf("in default case");
		}
		flush_fb();
	}
	else{
		printf("failed to read buttons!");
	}

}

	
void error_exit(void)
{
	close(fbfd);
	close(GPDD);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	printf("Hello World, I'm game!\n");
	
	screen.dx = 0;
	screen.dy = 0;
	screen.width = scr_width;
	screen.height = scr_height;
	
	fbfd = open("/dev/fb0", O_RDWR);
	if(fbfd == -1){
		printf("can't open frambuffer driver!!!\n");
		}
		
	int screensize = scr_width * scr_height * scr_BPP;
	framebuffer = (uint16_t*) mmap(NULL, screensize, PROT_READ| PROT_WRITE, MAP_SHARED, fbfd,0);
	
	GPDD = open("/dev/gamepad", O_RDWR);
	if(GPDD == -1){
		printf("Can't open gamepad driver!! Exiting... \n");
		exit(EXIT_FAILURE);
	}
	if(signal(SIGIO, &button_push) == SIG_ERR){
		printf("can't set up signal handler!");
		error_exit();
	}	
	if(fcntl(GPDD, F_SETOWN, getpid()) == -1){
		printf("Failed to set owner of device!");
		error_exit();
	}
	long flags = fcntl(GPDD, F_GETFL);
	if(fcntl(GPDD, F_SETFL, flags| FASYNC) ==-1){
		printf("error setting flags!");
		error_exit();
	}
	while(1)
	{
	
	
	}
		
	
	
	exit(EXIT_SUCCESS);
}


