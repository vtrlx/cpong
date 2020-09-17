/*
 * pong.c
 * (c) 2020 Victoria Lacroix
 *
 * A pong game in suckless-style.
 *
 * For information about this program, see README.txt
 * For copyright information, see COPYING.txt
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <png.h>

#define PX_SCALE 2
#define RESOLUTION_X 320
#define RESOLUTION_Y 200
#define STEPS_PER_FRAME 8
#define SUB_PX_SIZE 256

typedef enum Button {
	BUTTON_UP, BUTTON_DOWN,
	AMOUNT_BUTTONS
} Button;

typedef struct Image {
	unsigned int width, height;
	unsigned int subimage_width, subimage_height;
	GLuint texture;
} Image;

typedef struct PongObject {
	int x, y;
	int velocity_x, velocity_y;
	int width, height;
} PongObject;

static Image image_load(const char *path, unsigned int iw, unsigned int ih);

static int init(void);
static int pongobject_intersection(PongObject a, PongObject b);

static void ball_reset(void);
static void char_draw(char c, int x, int y);
static void control(void);
static void image_draw(Image i, int x, int y, int ix, int iy);
static void key_callback(struct GLFWwindow *w, int k, int sc, int a, int m);
static void next_input(void);
static void paint(void);
static void pongobject_draw(PongObject o);
static void pongobject_update(PongObject *o);
static void step(void);
static void text_draw(const char *txt, int x, int y);
static void update(void);

static const char *instructions =
	"          INSTRUCTIONS\n"
	"          ============\n"
	"\n"
	"   <UP> : Move Paddle Up\n"
	" <DOWN> : Move Paddle Down\n"
	"  <ESC> : Quit Game\n"
	"<ENTER> : Start Game\n";

static Image font;
static PongObject ball;
static PongObject opponent;
static PongObject player;

static char text_buffer[BUFSIZ];
static int input_buffer[AMOUNT_BUTTONS];
static int score_opponent, score_player;
static int wait_time;
static unsigned int started;
static unsigned int ticks;

static struct GLFWwindow *window;

static int
init(void)
{
	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	window = glfwCreateWindow(RESOLUTION_X * PX_SCALE,
		RESOLUTION_Y * PX_SCALE, "pong", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 2;
	}
	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glOrtho(0, RESOLUTION_X, RESOLUTION_Y, 0, -1, 1);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	font = image_load("res/asciifont.png", 5, 10);

	ball = (PongObject) {
		.width = 8 * SUB_PX_SIZE, .height = 8 * SUB_PX_SIZE,
	};
	opponent = (PongObject) {
		.x = (RESOLUTION_X - 24) * SUB_PX_SIZE, .y = 70 * SUB_PX_SIZE,
		.width = 8 * SUB_PX_SIZE, .height = 60 * SUB_PX_SIZE,
	};
	player = (PongObject) {
		.x = 16 * SUB_PX_SIZE, .y = 70 * SUB_PX_SIZE,
		.width = 8 * SUB_PX_SIZE, .height = 60 * SUB_PX_SIZE,
	};
	srand(time(0));

	return 0;
}

static int
pongobject_intersection(PongObject a, PongObject b)
{
	if (a.x + a.width > b.x && a.x < b.x + b.width &&
		a.y + a.height > b.y && a.y < b.y + b.height) {
		return 1;
	}
	return 0;
}

static Image
image_load(const char *path, unsigned int iw, unsigned int ih)
{
	FILE *fp;
	Image img = {0};
	int bit_depth, color_type;
	int rowbytes;
	png_byte header[8];
	png_infop info_ptr, end_info;
	png_structp png_ptr;
	unsigned int i;

	img.subimage_width = iw;
	img.subimage_height = ih;

	fp = fopen(path, "rb");
	if (!fp)
		return (Image){ 0 };

	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
		goto cleanup;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
		NULL, NULL);
	if (!png_ptr)
		goto cleanup;

	info_ptr = png_create_info_struct(png_ptr);
	end_info = png_create_info_struct(png_ptr);
	if (!info_ptr || !end_info || setjmp(png_jmpbuf(png_ptr)))
		goto cleanup;

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &img.width, &img.height, &bit_depth,
		&color_type, NULL, NULL, NULL);
	png_read_update_info(png_ptr, info_ptr);
	rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	rowbytes += 3 - ((rowbytes-1) % 4);
	png_byte *image_data;
	image_data = malloc(rowbytes * img.height * sizeof(png_byte) + 15);
	if (!image_data)
		goto cleanup;

	png_bytep *row_pointers = malloc(img.height * sizeof(png_bytep));
	if (!row_pointers)
		goto cleanup;
	for (i = 0; i < img.height; i++)
		row_pointers[img.height - 1 - i] = image_data + i * rowbytes;
	png_read_image(png_ptr, row_pointers);

	glGenTextures(1, &img.texture);
	glBindTexture(GL_TEXTURE_2D, img.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	free(row_pointers);

	img.width -= img.width % img.subimage_width;
	img.height -= img.height % img.subimage_height;
cleanup:
	if (fp)
		fclose(fp);
	if (png_ptr || info_ptr || end_info)
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	if (image_data)
		free(image_data);
	return img;
}

static void
ball_reset(void)
{
	ball.x = RESOLUTION_X * SUB_PX_SIZE / 2 - ball.width / 2;
	ball.y = RESOLUTION_Y * SUB_PX_SIZE / 2 - ball.height / 2;
	ball.velocity_x = 0;
	ball.velocity_y = 0;
	snprintf(text_buffer, BUFSIZ, "Score: %d - %d", score_player,
		score_opponent);
	wait_time = 180;
}

static void
char_draw(char c, int x, int y)
{
	unsigned int ix, iy;
	x *= font.subimage_width;
	y *= font.subimage_height;
	c -= ' ';
	ix = c % (font.width / font.subimage_width);
	iy = c / (font.width / font.subimage_width);
	image_draw(font, x, y, ix, iy);
}

static void
control(void)
{
	if (!started)
		return;

	int aim_y = ball.y + (ball.height / 2);
	if (opponent.y + (opponent.height * 0.25) > aim_y) {
		opponent.velocity_y = -2;
		if (ball.x >= RESOLUTION_X * SUB_PX_SIZE / 2)
			opponent.velocity_y--;
		opponent.velocity_y *= SUB_PX_SIZE;
	}
	else if (opponent.y + (opponent.height * 0.75) < aim_y) {
		opponent.velocity_y = 2;
		if (ball.x >= RESOLUTION_X * SUB_PX_SIZE / 2)
			opponent.velocity_y++;
		opponent.velocity_y *= SUB_PX_SIZE;
	}
	else {
		opponent.velocity_y = 0;
	}

	if (input_buffer[BUTTON_UP])
		player.velocity_y = -2 * SUB_PX_SIZE;
	else if (input_buffer[BUTTON_DOWN])
		player.velocity_y = 2 * SUB_PX_SIZE;
	else
		player.velocity_y = 0;
}

static void
image_draw(Image img, int x, int y, int ix, int iy)
{
	int iw = img.subimage_width;
	int ih = img.subimage_height;
	float fiw = iw / (float)img.width;
	float fih = -ih / (float)img.height;
	glBindTexture(GL_TEXTURE_2D, img.texture);
	glBegin(GL_QUADS);
	glTexCoord2f(ix * fiw, iy * fih);
	glVertex2i(x, y);
	glTexCoord2f(ix * fiw + fiw, iy * fih);
	glVertex2i(x + iw, y);
	glTexCoord2f(ix * fiw + fiw, iy * fih + fih);
	glVertex2i(x + iw, y + ih);
	glTexCoord2f(ix * fiw, iy * fih + fih);
	glVertex2i(x, y + ih);
	glEnd();
}

static void
key_callback(struct GLFWwindow *w, int k, int sc, int a, int m)
{
	int button;
	int newstate;

	if (k == GLFW_KEY_ESCAPE && a == GLFW_PRESS) {
		glfwSetWindowShouldClose(w, GLFW_TRUE);
		return;
	}
	else if (k == GLFW_KEY_ENTER && a == GLFW_PRESS) {
		started = 1;
		ball_reset();
		return;
	}

	if (a == GLFW_PRESS)
		newstate = 1;
	else if (a == GLFW_RELEASE)
		newstate = 0;
	else
		return;

	switch (k) {
	case GLFW_KEY_UP:
		button = BUTTON_UP;	
		break;
	case GLFW_KEY_DOWN:
		button = BUTTON_DOWN;	
		break;
	default:
		return;
	}

	input_buffer[button] = newstate;
}

static void
paint(void)
{
	if (!started) {
		text_draw(instructions, 4, 4);
		return;
	}
	pongobject_draw(ball);
	pongobject_draw(opponent);
	pongobject_draw(player);
	if (wait_time)
		text_draw(text_buffer, 12, 2);
}

static void
pongobject_draw(PongObject po)
{
	int x = po.x / SUB_PX_SIZE;
	int y = po.y / SUB_PX_SIZE;
	int w = po.width / SUB_PX_SIZE;
	int h = po.height / SUB_PX_SIZE;
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	glVertex2i(x, y);
	glVertex2i(x + w, y);
	glVertex2i(x + w, y + h);
	glVertex2i(x, y + h);
	glEnd();
}

static void
pongobject_update(PongObject *o)
{
	if (!o)
		return;
	o->x += o->velocity_x / STEPS_PER_FRAME;
	o->y += o->velocity_y / STEPS_PER_FRAME;
	if (o->y < 0)
		o->y = 0;
	if (o->y + o->height >= RESOLUTION_Y * SUB_PX_SIZE)
		o->y = RESOLUTION_Y * SUB_PX_SIZE - o->height;
}

static void
text_draw(const char *txt, int x, int y)
{
	int tx = x, ty = y;
	unsigned int i;
	if (!txt)
		return;
	for (i = 0; txt[i]; i++) {
		if (txt[i] >= ' ') {
			char_draw(txt[i], tx, ty);
			tx++;
		}
		else if(txt[i] == '\n') {
			tx = x;
			ty++;
		}
	}
}

static void
update(void)
{
	if (!started)
		return;

	/* sets the ball's velocity if it stops */
	if (!ball.velocity_x) {
		ball.velocity_x = (rand() % 2) ? -1 : 1;
		ball.velocity_x *= SUB_PX_SIZE;
	}
	if (!ball.velocity_y) {
		ball.velocity_y = (rand() % 2) ? -1 : 1;
		ball.velocity_y *= SUB_PX_SIZE;
	}

	/* clamp the ball's velocity if it's too fast */
	if (ball.velocity_x < -4 * SUB_PX_SIZE)
		ball.velocity_x = -4 * SUB_PX_SIZE;
	if (ball.velocity_x > 4 * SUB_PX_SIZE)
		ball.velocity_x = 4 * SUB_PX_SIZE;
	if (ball.velocity_y < -4 * SUB_PX_SIZE)
		ball.velocity_y = -4 * SUB_PX_SIZE;
	if (ball.velocity_y > 4 * SUB_PX_SIZE)
		ball.velocity_y = 4 * SUB_PX_SIZE;

	pongobject_update(&opponent);
	pongobject_update(&player);
	if (!wait_time)
		pongobject_update(&ball);

	/* calculate the ball hitting the boundaries */
	if (ball.x + ball.width <= 0) {
		score_opponent++;
		ball_reset();
	}
	else if (ball.x >= RESOLUTION_X * SUB_PX_SIZE) {
		score_player++;
		ball_reset();
	}
	else if (ball.y <= 0) {
		ball.velocity_y = -ball.velocity_y;
		ball.y = 0;
	}
	else if (ball.y + ball.height >= RESOLUTION_Y * SUB_PX_SIZE) {
		ball.velocity_y = -ball.velocity_y;
		ball.y = RESOLUTION_Y * SUB_PX_SIZE - ball.height;
	}

	/* calculate ball hitting the paddle */
	if (ball.velocity_x < 0 && ball.x >= player.x &&
		pongobject_intersection(player, ball)) {
		if (ball.velocity_y > 0)
			ball.velocity_y += SUB_PX_SIZE;
		else
			ball.velocity_y -= SUB_PX_SIZE;
		ball.velocity_x = -ball.velocity_x;
		ball.velocity_x += SUB_PX_SIZE;
		ball.x = player.x + player.width;
	}
	else if (ball.velocity_x > 0 && ball.x <= opponent.x + opponent.width
		&& pongobject_intersection(opponent, ball)) {
		if (ball.velocity_y > 0)
			ball.velocity_y += SUB_PX_SIZE;
		else
			ball.velocity_y -= SUB_PX_SIZE;
		ball.velocity_x = -ball.velocity_x;
		ball.velocity_x -= SUB_PX_SIZE;
		ball.x = opponent.x - ball.width;
	}
}

int
main(int argc, char *argv[])
{
	int i;
	int ret;
	ret = init();
	if (ret)
		return ret;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		control();
		for (i = 0; i < STEPS_PER_FRAME; ++i)
			update();
		if (wait_time)
			wait_time--;
		glClear(GL_COLOR_BUFFER_BIT);
		paint();
		glfwSwapBuffers(window);
		ticks++;
	}
	return 0;
}
