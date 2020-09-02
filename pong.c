/*
 * pong.c
 *
 * (c) 2020 Victoria Lacroix
 *
 * a pong game in suckless-style
 *
 * please read the README for more details
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

#define PX_SCALE 3
#define RESOLUTION_X 256
#define RESOLUTION_Y 192
#define STEPS_PER_FRAME 8
#define SUB_PX_SIZE 256

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

static int init(void);
static int pongobject_intersection(PongObject a, PongObject b);
static void key_callback(struct GLFWwindow *w, int k, int sc, int a, int m);
static void paint(void);
static void pongobject_draw(PongObject o);
static void pongobject_update(PongObject *o);
static void step(void);
static void update(void);

static Image font;
static PongObject ball;
static PongObject opponent;
static PongObject player;
static char text_buffer[BUFSIZ];
static int opponent_score, player_score;
static int wait_time;
static unsigned int ticks;

static struct GLFWwindow *window;

/*
 * initializes parts of the game state
 */
static int
init(void)
{
	/* glfw3 init */
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

	/* OpenGL init */
	glOrtho(0, RESOLUTION_X, RESOLUTION_Y, 0, -1, 1);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/* game state init */
	ball = (PongObject) {
		.x = 128 * SUB_PX_SIZE, .y = 96 * SUB_PX_SIZE, .width = 8 * SUB_PX_SIZE, .height = 8 * SUB_PX_SIZE,
	};
	opponent = (PongObject) {
		.x = 232 * SUB_PX_SIZE, .y = 64 * SUB_PX_SIZE,
		.width = 8 * SUB_PX_SIZE, .height = 60 * SUB_PX_SIZE,
	};
	player = (PongObject) {
		.x = 16 * SUB_PX_SIZE, .y = 64 * SUB_PX_SIZE,
		.width = 8 * SUB_PX_SIZE, .height = 60 * SUB_PX_SIZE,
	};
	wait_time = 300;
	srand(time(0));

	return 0;
}

/*
 * checks if two pongobjects are intersecting
 */
static int
pongobject_intersection(PongObject a, PongObject b)
{
	if (a.x + a.width > b.x && a.x < b.x + b.width &&
		a.y + a.height > b.y && a.y < b.y + b.height) {
		return 1;
	}
	return 0;
}

/*
 * loads an image file for rendering
 */
static Image
image_load(const char *path, unsigned int subimage_width, unsigned int subimage_height)
{
	FILE *fp;
	int bit_depth, color_type;
	int rowbytes;
	png_byte header[8];
	png_infop info_ptr, end_info;
	png_structp png_ptr;
	Image img = {0};
	unsigned int i;
	fp = fopen(path, "rb");
	if (!fp)
		return (Image){ 0 };
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
		goto cleanup;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		goto cleanup;
	info_ptr = png_create_info_struct(png_ptr);
	end_info = png_create_info_struct(png_ptr);
	if (!info_ptr || !end_info || setjmp(png_jmpbuf(png_ptr)))
		goto cleanup;
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &img.width, &img.height, &bit_depth, &color_type, NULL, NULL, NULL);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	free(row_pointers);
	img.subimage_width = subimage_width;
	img.subimage_height = subimage_height;
	img.width -= img.width % subimage_width;
	img.height -= img.height % subimage_height;
cleanup:
	if (fp != NULL) fclose(fp);
	if (png_ptr != NULL || info_ptr != NULL || end_info != NULL)
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	if (image_data != NULL) free(image_data);
	return img;
}

/*
 * resets the ball's position
 */
static void
ball_reset()
{
	ball.x = 128 * SUB_PX_SIZE;
	ball.y = 96 * SUB_PX_SIZE;
	ball.velocity_x = 0;
	ball.velocity_y = 0;
}

/*
 * called by GLFW whenever a keypress is registered
 * updates the state of the buttons array depending on key pressed
 * quits if escape is pressed
 */
static void
key_callback(struct GLFWwindow *w, int k, int sc, int a, int m)
{
	if (k == GLFW_KEY_ESCAPE && a == GLFW_PRESS) {
		glfwSetWindowShouldClose(w, GLFW_TRUE);
		return;
	}
	switch (k) {
	case GLFW_KEY_UP:
	case GLFW_KEY_W:
		if (a == GLFW_PRESS) {
			player.velocity_y = -512;
		}
		else if (a == GLFW_RELEASE) {
			player.velocity_y = 0;
		}
		break;
	case GLFW_KEY_DOWN:
	case GLFW_KEY_S:
		if (a == GLFW_PRESS) {
			player.velocity_y = 512;
		}
		else if (a == GLFW_RELEASE) {
			player.velocity_y = 0;
		}
		break;
	default:
		return;
	}
}

/*
 * Draws an image to the screen.
 */
static void
image_draw(Image img, int x, int y, unsigned int ix, unsigned int iy)
{
	float iw = (float) img.subimage_width / img.width;
	float ih = -(float) img.subimage_height / img.height;
	glBindTexture(GL_TEXTURE_2D, img.texture);
	glBegin(GL_QUADS);
	glTexCoord2f(ix * iw, iy * ih);
	glVertex2f(x, y);
	glTexCoord2f(ix * iw + iw, iy * ih);
	glVertex2f(x + (int)img.subimage_width, y);
	glTexCoord2f(ix * iw + iw, iy * ih + ih);
	glVertex2f(x + (int)img.subimage_width, y + (int)img.subimage_height);
	glTexCoord2f(ix * iw, iy * ih + ih);
	glVertex2f(x, y + (int)img.subimage_height);
	glEnd();
}

/*
 * renders the game
 */
static void
paint(void)
{
	if (wait_time) {
		/* paint text */
	}
	pongobject_draw(ball);
	pongobject_draw(opponent);
	pongobject_draw(player);
}

/*
 * draws a pong object
 */
static void
pongobject_draw(PongObject po)
{
	float x = po.x / (float)SUB_PX_SIZE;
	float y = po.y / (float)SUB_PX_SIZE;
	float w = po.width / (float)SUB_PX_SIZE;
	float h = po.height / (float)SUB_PX_SIZE;
	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);
	glEnd();
}

/*
 * updates parts of the game state that are independent of player input
 */
static void
update(void)
{
	/* opponent tries to follow the ball */
	if (opponent.y + (opponent.height * 0.25) > ball.y + (ball.height / 2)) {
		opponent.velocity_y = (ball.x >= RESOLUTION_X * SUB_PX_SIZE / 2) ? -3 : -2;
		opponent.velocity_y *= SUB_PX_SIZE;
	}
	else if (opponent.y + (opponent.height * 0.75) < ball.y + (ball.height / 2)) {
		opponent.velocity_y = (ball.x >= RESOLUTION_X * SUB_PX_SIZE / 2) ? 3 : 2;
		opponent.velocity_y *= SUB_PX_SIZE;
	}
	else {
		opponent.velocity_y = 0;
	}

	/* sets the ball's velocity if it stops */
	if (!ball.velocity_x)
		ball.velocity_x = (rand() % 2) ? -1 * SUB_PX_SIZE : 1 * SUB_PX_SIZE;
	if (!ball.velocity_y)
		ball.velocity_y = (rand() % 2) ? -1 * SUB_PX_SIZE : 1 * SUB_PX_SIZE;

	opponent.y += opponent.velocity_y / STEPS_PER_FRAME;
	player.y += player.velocity_y / STEPS_PER_FRAME;

	if (!wait_time) {
		ball.x += ball.velocity_x / STEPS_PER_FRAME;
		ball.y += ball.velocity_y / STEPS_PER_FRAME;
	}

	/* clamp the ball's velocity */
	ball.velocity_y = (ball.velocity_y < -4 * SUB_PX_SIZE) ? -4 * SUB_PX_SIZE :
		ball.velocity_y;
	ball.velocity_y = (ball.velocity_y > 4 * SUB_PX_SIZE) ? 4 * SUB_PX_SIZE :
		ball.velocity_y;
	ball.velocity_x = (ball.velocity_x < -4 * SUB_PX_SIZE) ? -4 * SUB_PX_SIZE :
		ball.velocity_x;
	ball.velocity_x = (ball.velocity_x > 4 * SUB_PX_SIZE) ? 4 * SUB_PX_SIZE :
		ball.velocity_x;

	/* calculate the ball hitting the boundaries */
	if (ball.x < 0) {
		/* score_opponent++; */
		wait_time = 300;
		ball_reset();
	}
	else if (ball.x + ball.width > RESOLUTION_X * SUB_PX_SIZE) {
		/* score_player++; */
		wait_time = 300;
		ball_reset();
	}
	else if (ball.y < 0) {
		ball.velocity_y = -ball.velocity_y;
		ball.y = 0;
	}
	else if (ball.y + ball.height > RESOLUTION_Y * SUB_PX_SIZE) {
		ball.velocity_y = -ball.velocity_y;
		ball.y = RESOLUTION_Y * SUB_PX_SIZE - ball.height;
	}

	/* calculate ball hitting the paddle */
	if (ball.velocity_x < 0 && pongobject_intersection(player, ball)) {
		ball.velocity_y += (ball.velocity_y > 0) ? SUB_PX_SIZE : -SUB_PX_SIZE;
		ball.velocity_x = -ball.velocity_x;
		ball.velocity_x += SUB_PX_SIZE;
		ball.x = player.x + player.width;
	}
	else if (ball.velocity_x > 0 && pongobject_intersection(opponent, ball)) {
		ball.velocity_y += (ball.velocity_y > 0) ? SUB_PX_SIZE : -SUB_PX_SIZE;
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
		glClear(GL_COLOR_BUFFER_BIT);
		for (i = 0; i < STEPS_PER_FRAME; ++i)
			update();
		if (wait_time)
			wait_time--;
		paint();
		glfwSwapBuffers(window);
		ticks++;
	}
	return 0;
}
